#include "mcv_platform.h"
#include "deferred_renderer.h"
#include "render_manager.h"
#include "render/render_module.h"
#include "engine.h"
#include "draw_primitives.h"
#include "textures/render_to_texture.h"
#include "components/common/comp_light_point.h"
#include "components/common/comp_light_spot.h"
#include "components/postfx/comp_render_ao.h"

CDeferredRenderer::CDeferredRenderer() {
}

// -----------------------------------------------------------------
bool CDeferredRenderer::create(int new_xres, int new_yres, bool new_irradiance_cache) {
	PROFILE_FUNCTION("CDeferredRenderer::create");

	irradiance_cache = new_irradiance_cache;
	xres = new_xres;
	yres = new_yres;
	dbg("Initializing deferred to %dx%d\n", xres, yres);

	destroy();

	if (!rt_albedos) {
		rt_albedos = new CRenderToTexture;
		rt_normals = new CRenderToTexture;
		rt_normals_aux = new CRenderToTexture;
		rt_depth = new CRenderToTexture;
		rt_emissive = new CRenderToTexture;
	}

	std::string res = std::to_string(new_xres) + "x" + std::to_string(new_yres);

	if (!rt_albedos->createRT((res + std::string("g_albedos.dds")).c_str(), xres, yres, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_UNKNOWN, true, irradiance_cache, false, 1, true))
		return false;
	if (!rt_normals->createRT((res + std::string("g_normals.dds")).c_str(), xres, yres, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_UNKNOWN, false, false, false, 1, true))
		return false;
	if (!rt_normals_aux->createRT((res + std::string("g_normals_aux.dds")).c_str(), xres, yres, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_UNKNOWN, false, false, false, 1, true))
		return false;
	if (!rt_depth->createRT((res + std::string("g_depths.dds")).c_str(), xres, yres, DXGI_FORMAT_R32_FLOAT, DXGI_FORMAT_UNKNOWN, false, false, false, 1, true))
		return false;
	if (!rt_emissive->createRT((res + std::string("g_emissive.dds")).c_str(), xres, yres, DXGI_FORMAT_R16G16B16A16_FLOAT))
		return false;

	if (!irradiance_cache) {
		PROFILE_FUNCTION("ASSO");
		CMemoryDataProvider mdp("data/shaders/ASSAO.hlsl");
		ASSAO_CreateDescDX11 desc(Render.device, mdp.data(), mdp.size());
		assao_fx = ASSAO_Effect::CreateInstance(&desc);
		assert(assao_fx);
	}

	return true;
}

// -----------------------------------------------------------------
void CDeferredRenderer::destroy() {

	if (rt_albedos) {
		rt_albedos->destroy();
		rt_normals->destroy();
		rt_normals_aux->destroy();
		rt_emissive->destroy();
		rt_depth->destroy();
	}

	if (assao_fx) {
		ASSAO_Effect::DestroyInstance(assao_fx);
		assao_fx = nullptr;
	}

}

void CDeferredRenderer::renderGBuffer() {
	PROFILE_FUNCTION("GBuffer");
	CGpuScope gpu_scope("Deferred.GBuffer");

	// Disable the gbuffer textures as we are going to update them
	// Can't render to those textures and have them active in some slot...
	CTexture::deactivate(TS_DEFERRED_ALBEDOS);
	CTexture::deactivate(TS_DEFERRED_NORMALS);
	CTexture::deactivate(TS_DEFERRED_LINEAR_DEPTH);
	CTexture::deactivate(TS_DEFERRED_EMISSIVES);

	// Activate el multi-render-target MRT
	const int nrender_targets = 4;
	ID3D11RenderTargetView* rts[nrender_targets] = {
	  rt_albedos->getRenderTargetView(),
	  rt_normals->getRenderTargetView(),
	  rt_depth->getRenderTargetView(),
	  rt_emissive->getRenderTargetView()
	};

	// We use our N rt's and the Zbuffer of the backbuffer
	Render.ctx->OMSetRenderTargets(nrender_targets, rts, irradiance_cache ? Render.depth_stencil_view_irradiance : Render.depth_stencil_view);
	rt_albedos->activateViewport();   // Any rt will do...

	// Clear output buffers. some can be removed if we intend to fill all the screen
	// with new data.
	rt_albedos->clear(VEC4(1, 0, 0, 1));
	rt_normals->clear(VEC4(0, 0, 1, 1));
	rt_depth->clear(VEC4(100));
	rt_emissive->clear(VEC4(0, 0, 0, 1));

	// Clear ZBuffer with the value 1.0 (far)
	Render.ctx->ClearDepthStencilView(irradiance_cache ? Render.depth_stencil_view_irradiance : Render.depth_stencil_view, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	RenderManager.renderAll(eRenderChannel::RC_SOLID, h_camera);
	RenderManager.renderAll(eRenderChannel::RC_SOLID_STENCIL, h_camera);

	// Disable rendering to all render targets.
	CRenderToTexture::deactivate(nrender_targets);

	// Activate the gbuffer textures to other shaders
	rt_albedos->activate(TS_DEFERRED_ALBEDOS);
	rt_normals->activate(TS_DEFERRED_NORMALS);
	rt_depth->activate(TS_DEFERRED_LINEAR_DEPTH);
	rt_emissive->activate(TS_DEFERRED_EMISSIVES);
}

// -----------------------------------------------------------------
void CDeferredRenderer::renderAmbientPass() {
	if (irradiance_cache) return;
	CGpuScope gpu_scope("renderAmbientPass");
	drawFullScreenQuad("pbr_ambient.pipeline", nullptr);
}

// -----------------------------------------------------------------
void CDeferredRenderer::renderEmissivePass() {
	CGpuScope gpu_scope("renderEmissivePass");
	drawFullScreenQuad("pbr_emissive.pipeline", nullptr);

	// Render ONLY emissive (no other pipeline combined)
	renderAutoEmissives();
}

// -----------------------------------------------------------------
void CDeferredRenderer::renderPointLights() {
	CGpuScope gpu_scope("renderPointLights");

	// What to render
	auto mesh = Resources.get("data/meshes/UnitSolidSphere.mesh")->as<CMesh>();
	mesh->activate();

	// How to render
	auto pipeline = Resources.get("pbr_light_point.pipeline")->as<CPipelineState>();
	pipeline->activate();

	getObjectManager<TCompLightPoint>()->forEach([&](TCompLightPoint* light_point) {

		// Activate light
		if (!light_point->activate())
			return;

		// Where to render
		activateObject(light_point->getWorld());

		// Do the render
		mesh->render();
	});
}

// -----------------------------------------------------------------
void CDeferredRenderer::renderSpotLights(bool with_shadows) {
	CGpuScope gpu_scope(with_shadows ? "renderSpotLightsShadows" : "renderSpotLights");

	// What to render
	auto mesh = Resources.get("data/meshes/ViewVolumeSolid.mesh")->as<CMesh>();
	mesh->activate();

	//  // How to render
	const char* name = with_shadows ? "pbr_light_spot_shadows.pipeline" : "pbr_light_spot.pipeline";
	auto pipeline = Resources.get(name)->as<CPipelineState>();
	pipeline->activate();

	getObjectManager<TCompLightSpot>()->forEach([&](TCompLightSpot* light_spot) {

		if (light_spot->shadows_enabled != with_shadows)
			return;

		// Activate light
		if (!light_spot->activate())
			return;

		// Where to render
		activateObject(light_spot->getWorld());

		// Do the render
		mesh->render();
	});
}

// ---------------------------------------------------------------
void CDeferredRenderer::renderSkyBox() {
	CGpuScope gpu_scope("renderSkyBox");
	drawFullScreenQuad("pbr_skybox.pipeline", nullptr);
}

// ---------------------------------------------------------------
void CDeferredRenderer::renderAutoEmissives() {
  RenderManager.renderAll(eRenderChannel::RC_EMISSIVE, h_camera);
}

// ---------------------------------------------------------------
void CDeferredRenderer::renderAO() {

	if (irradiance_cache) return;
	CEntity* e_camera = h_camera;
	if (!e_camera) return;

	assert(e_camera);
	TCompRenderAO* comp_ao = e_camera->get<TCompRenderAO>();

	const CTexture* ao_texture = nullptr;
	if (comp_ao) {
		CTexture::deactivate(TS_DEFERRED_AO);
		ao_texture = comp_ao->compute(rt_normals, assao_fx);
	}

	// As there is no comp AO, or it's not eanbled use a white texture as substitute
	if (!ao_texture)
		ao_texture = Resources.get("data/textures/white.dds")->as<CTexture>();

	// Leave the texture in slot TS_DEFERRED_AO
	ao_texture->activate(TS_DEFERRED_AO);
}

void CDeferredRenderer::renderDecals()
{
	CTexture::deactivate(TS_DEFERRED_ALBEDOS);
	CTexture::deactivate(TS_DEFERRED_NORMALS);
	CTexture::deactivate(TS_DEFERRED_EMISSIVES);

	// Clone texture (method to use the stencil mask)
	rt_normals_aux->copyFromResource(rt_normals);

	// Activate el multi-render-target MRT
	const int nrender_targets = 3;
	ID3D11RenderTargetView* rts[nrender_targets] = {
	  rt_albedos->getRenderTargetView(),
	  rt_normals->getRenderTargetView(),
	  rt_emissive->getRenderTargetView()
	};

	/*const int n_uavs = 1;
	ID3D11UnorderedAccessView* uavs[n_uavs] = {
	  rt_normals->getUAV()
	};*/

	// UINT initialCounts[1] = { -1 };
	// Render.ctx->OMSetRenderTargetsAndUnorderedAccessViews(nrender_targets, rts, Render.depth_stencil_view, nrender_targets, n_uavs, uavs, initialCounts);

	Render.ctx->OMSetRenderTargets(nrender_targets, rts, Render.depth_stencil_view);
	rt_albedos->activateViewport();
	rt_normals_aux->activate(TS_DEFERRED_NORMALS);

	RenderManager.renderAll(eRenderChannel::RC_DECALS, h_camera);

	// Disable rendering to all render targets.
	CRenderToTexture::deactivate(nrender_targets);

	// Activate the gbuffer textures to other shaders
	rt_albedos->activate(TS_DEFERRED_ALBEDOS);
	rt_normals->activate(TS_DEFERRED_NORMALS);
	rt_emissive->activate(TS_DEFERRED_EMISSIVES);
}

void CDeferredRenderer::renderWater()
{
	CTexture::deactivate(TS_DEFERRED_ALBEDOS);
	CTexture::deactivate(TS_DEFERRED_LINEAR_DEPTH);
	CTexture::deactivate(TS_DEFERRED_NORMALS);
	CTexture::deactivate(TS_DEFERRED_EMISSIVES);

	// Activate el multi-render-target MRT
	const int nrender_targets = 1;
	ID3D11RenderTargetView* rts[nrender_targets] = {
	  rt_emissive->getRenderTargetView()
	};

	const int n_uavs = 3;
	ID3D11UnorderedAccessView* uavs[n_uavs] = {
      rt_albedos->getUAV(),
	  rt_normals->getUAV(),
	  rt_depth->getUAV()
	};

	Render.ctx->OMSetRenderTargetsAndUnorderedAccessViews(nrender_targets, rts, irradiance_cache ? Render.depth_stencil_view_irradiance : Render.depth_stencil_view, nrender_targets, n_uavs, uavs, nullptr);
	rt_albedos->activateViewport();

	RenderManager.renderAll(eRenderChannel::RC_WATER, h_camera);

	ID3D11RenderTargetView* rts_null[nrender_targets] = { nullptr };
	ID3D11UnorderedAccessView* uavs_null[n_uavs] = { nullptr, nullptr,  nullptr };
	Render.ctx->OMSetRenderTargetsAndUnorderedAccessViews(nrender_targets, rts_null, nullptr, nrender_targets, n_uavs, uavs_null, nullptr);

	// Activate the gbuffer textures to other shaders
	rt_albedos->activate(TS_DEFERRED_ALBEDOS);
	rt_depth->activate(TS_DEFERRED_LINEAR_DEPTH);
	rt_normals->activate(TS_DEFERRED_NORMALS);
	rt_emissive->activate(TS_DEFERRED_EMISSIVES);
}

// ---------------------------------------------------------------
void CDeferredRenderer::render(CRenderToTexture* out_rt, CHandle new_h_camera, int array_index)
{
  h_camera = new_h_camera;
  assert(out_rt);

  PROFILE_FUNCTION("Deferred");
  renderGBuffer();
  renderWater();
  renderDecals();
  renderAO();

  out_rt->activateRT(nullptr, array_index);
  renderAmbientPass();
  renderPointLights();
  renderSpotLights(false);
  renderSpotLights(true);
  renderEmissivePass();
  renderSkyBox();
}