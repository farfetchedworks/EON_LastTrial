#include "mcv_platform.h"
#include "engine.h"
#include "render/textures/image.h"
#include "comp_irradiance_cache.h"
#include "components/common/comp_transform.h"
#include "components/common/comp_camera.h"
#include "components/common/comp_light_point.h"
#include "components/common/comp_light_spot.h"
#include "components/common/comp_compute.h"
#include "render/draw_primitives.h"
#include "render/render_manager.h"
#include "modules/game/module_fluid_simulation.h"
#include "render/render_module.h"
#include "render/compute/gpu_buffer.h"
#include "render/compute/compute_shader.h"
#include "modules/module_physics.h"
#include "render/gpu_culling_module.h"
#include "entity/entity_parser.h"
#include "render/render.h"

#include <CommCtrl.h>
#pragma comment(lib, "Comctl32.lib")

// This prevents the progress bar to look like Windows 95
#pragma comment(linker,"\"/manifestdependency:type='win32' \
    name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
    processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

DECL_OBJ_MANAGER("irradiance_cache", TCompIrradianceCache);

extern CShaderCte<CtesCamera> cte_camera;
extern CShaderCte<CtesObject> cte_object;
extern CShaderCte<CtesLight> cte_light;
extern CShaderCte<CtesWorld> cte_world;

VEC3 cubemapFaceNormals[6][3] = {
	{{0, 0, -1}, {0, 1, 0}, {1, 0, 0}},		// posx
	{{0, 0, 1},  {0, 1, 0}, {-1, 0, 0}},	// negx

	{{1, 0, 0}, {0, 0, 1},  {0, 1, 0}},		// posy
	{{1, 0, 0}, {0, 0, -1}, {0, -1, 0}},	// negy

	{{1, 0, 0},  {0, 1, 0}, {0, 0, 1}},		// posz
	{{-1, 0, 0}, {0, 1, 0}, {0, 0, -1}}		// negz
};

bool TCompIrradianceCache::GLOBAL_RENDER_PROBES = false;
bool TCompIrradianceCache::RENDERING_ACTIVE = false;
float TCompIrradianceCache::GLOBAL_IRRADIANCE_MULTIPLIER = 1.0f;
float TCompIrradianceCache::EMISSIVE_MULTIPLIER = 10.0f;

void TCompIrradianceCache::setRenderingIrradianceEnabled(bool enabled)
{
	if (enabled) {
		GLOBAL_IRRADIANCE_MULTIPLIER = 10.0f;
		cte_world.emissive_irradiance_multiplier = EMISSIVE_MULTIPLIER;
	}
	else {
		GLOBAL_IRRADIANCE_MULTIPLIER = 1.0f;
		cte_world.emissive_irradiance_multiplier = 1.0f;
	}

	RENDERING_ACTIVE = enabled;
}

void TCompIrradianceCache::load(const json& j, TEntityParseContext& ctx)
{
	grid_dimensions = loadVEC3(j, "dimensions");
	grid_density = loadVEC3(j, "grid_density");
	irradianceFilename = j.value("filename", "irradiance.irr");
	GLOBAL_RENDER_PROBES = j.value("render_probes", GLOBAL_RENDER_PROBES);
}

void TCompIrradianceCache::initCache(int idx, CGPUBuffer** sh_buffer)
{
	cache_idx = idx;

	render_texture = new CRenderToTexture();
	//read_texture = new CTexture();

	render_texture->createRT("cubemaps_array.dds", IR_SIZE, IR_SIZE, DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_UNKNOWN,
		false, true, true, cubemap_batch_size * 6);

	deferred_renderer.create(IR_SIZE, IR_SIZE, true);

	h_transform = get<TCompTransform>();

	for (int index = 0; index < 6; ++index)
	{
		for (int v = 0; v < IR_SIZE; v++) {
			for (int u = 0; u < IR_SIZE; u++)
			{
				float fU = (2.f * u / (IR_SIZE - 1.f)) - 1.f;
				float fV = (2.f * v / (IR_SIZE - 1.f)) - 1.f;

				VEC3 vecX = cubemapFaceNormals[index][0] * fU;
				VEC3 vecY = cubemapFaceNormals[index][1] * fV;
				VEC3 vecZ = cubemapFaceNormals[index][2];

				VEC3 res = VEC3(vecX + vecY + vecZ);
				res.Normalize();
				cubeMapVecs[index * IR_SIZE * IR_SIZE + v * IR_SIZE + u] = VEC4(res);
			}
		}
	}

	TCompTransform* c_trans = h_transform;

	// When computing the probes position
	VEC3 scale = grid_dimensions;
	VEC3 position = c_trans->getPosition();

	// Define the corners of the axiswd  aligned grid
	// This can be done using the boundings of our scene
	start_pos = position - scale / 2;
	end_pos = position + scale / 2;

	start_pos.y += scale.y / 2.f; end_pos.y += scale.y / 2.f;

	// Compute the vector from one corner to the other
	delta = (end_pos - start_pos);

	// And scale it down according to the subdivisions
	// We substract one to be sure the last probe is at end pos
	delta.x /= (grid_density.x - 1);
	delta.y /= (grid_density.y - 1);
	delta.z /= (grid_density.z - 1);

	if (grid_density.x == 1 && grid_density.y == 1 && grid_density.z == 1)
		delta = VEC3::Zero;

	*sh_buffer = new CGPUBuffer();

	float bufferSize = 9.f * grid_density.x * grid_density.y * grid_density.z;
	(*sh_buffer)->create(sizeof(VEC4), (uint32_t)bufferSize, false);

	bool is_ok = loadFromFile(*sh_buffer);

	//if (!is_ok) {
	createGrid();
	//}

	updateWorldVars();
}

void TCompIrradianceCache::showInMenu(int idx, CGPUBuffer* sh_buffer)
{
	ImGui::Text("Num probes: %d (%d)", probes.size(), visible_probes_size);
	if (ImGui::SmallButton("Calculate Irradiance")) {
		renderProbeCubemaps(idx, sh_buffer);
	}

	ImGui::DragFloat("Emissive Multiplier", &EMISSIVE_MULTIPLIER, 1.f, 1.f, 200.0f);

	ImGui::Checkbox("Render Probes", &GLOBAL_RENDER_PROBES);
	//ImGui::Checkbox("Use Texture", &use_texture);
	// ImGui::Checkbox("Use Disabled Probes", &use_disabled);
	/*if(sh_texture)
		sh_texture->renderInMenu();*/
}

void TCompIrradianceCache::createGrid()
{
	probes.clear();

	// num_disabled = 0;

	int discarded_probes = 0;
	// Now delta give us the distance between probes in every axis
	// Compute the centers
	for (int z = 0; z < grid_density.z; ++z) {
		for (int y = 0; y < grid_density.y; ++y) {
			for (int x = 0; x < grid_density.x; ++x) {
				sProbe p;
				p.local = VEC3((float)x, (float)y, (float)z);

				// Index in the linear array
				//p.index = x + y * (int)grid_density.x + z * (int)grid_density.x * (int)grid_density.y;

				// And its position
				p.pos = start_pos + delta * VEC3((float)x, (float)y, (float)z);

#ifndef _DEBUG

				VHandles collisions;
				EnginePhysics.overlapSphere(p.pos, 2.0f, collisions, CModulePhysics::FilterGroup::Scenario);
				EnginePhysics.raycast(p.pos, VEC3(0, -1, 0), 5.0, collisions, CModulePhysics::FilterGroup::Scenario);
				if (collisions.empty()) {
					p.disabled = true;
					discarded_probes++;
				}
#endif

				// Discard probes
				/*{
					bool closeLight = false;

					getObjectManager<TCompLightPoint>()->forEach([&](TCompLightPoint* light_point) {
						CEntity* eLight = CHandle(light_point).getOwner();
						closeLight |= VEC3::Distance(eLight->getPosition(), p.pos) < light_point->radius;
					});

					getObjectManager<TCompLightSpot>()->forEach([&](TCompLightSpot* light_spot) {
						CEntity* eLight = CHandle(light_spot).getOwner();
						TCompTransform* lTransform = eLight->get<TCompTransform>();
						
						VEC3 lightToProbe = p.pos - eLight->getPosition();
						lightToProbe.Normalize();

						float dotf = lightToProbe.Dot(lTransform->getForward());
						closeLight |= (dotf > 0);
					});

					if (!closeLight) {
						num_disabled++;
						p.disabled = true;
					}
				}*/

				probes.push_back(p);
			}
		}
	}

	visible_probes_size = (int)probes.size() - discarded_probes;
}

void TCompIrradianceCache::updateWorldVars()
{
	cte_world.sh_data[cache_idx].sh_grid_delta = delta;
	cte_world.sh_data[cache_idx].sh_grid_dimensions = grid_density;
	cte_world.sh_data[cache_idx].sh_grid_start = start_pos;
	cte_world.sh_data[cache_idx].sh_grid_end = end_pos;
	cte_world.sh_data[cache_idx].sh_grid_num_probes = (int)probes.size();
}

void TCompIrradianceCache::renderProbeCubemaps(int idx, CGPUBuffer* sh_buffer)
{
	// Clear sh buffer to 0
	for (int i = 0; i < sh_buffer->cpu_data.size(); ++i) {
		sh_buffer->cpu_data[i] = 0;
	}

	sh_buffer->copyCPUtoGPU();

	// Create progress bar
	{
		RECT rcClient;  // Client area of parent window.
		int cyVScroll;  // Height of scroll bar arrow.

		InitCommonControls();

		HWND hwndParent = CApplication::get().getHandle();
		GetClientRect(hwndParent, &rcClient);

		cyVScroll = GetSystemMetrics(SM_CYVSCROLL);

		hwndPB = CreateWindowEx(0, PROGRESS_CLASS, (LPTSTR)NULL,
			WS_CHILD | WS_VISIBLE, rcClient.left,
			rcClient.bottom - cyVScroll,
			rcClient.right, cyVScroll,
			hwndParent, (HMENU)0, CApplication::get().getHInstance(), NULL);

		SendMessage(hwndPB, PBM_SETRANGE, 0, MAKELPARAM(0, ceil(probes.size() / (float)cubemap_batch_size) + 1));

		SendMessage(hwndPB, PBM_SETSTEP, (WPARAM)1, 0);
	}

	CGpuScope gpu_scope("calculateIrradiance");

	setRenderingIrradianceEnabled(true);

	CEntity* e_camera = getEntityByName("camera_cubemaps");
	TCompCamera* c_camera = e_camera->get<TCompCamera>();
	c_camera->setAspectRatio(1.0f);

	cte_camera.activate();
	cte_object.activate();
	cte_light.activate();
	cte_world.activate();
	cte_world.activateCS();

	CRenderModule::updateWorldCtes(IR_SIZE, IR_SIZE);

	// Generate cube map vectors
	ID3D11Buffer* null_buffer = nullptr;
	Render.ctx->VSSetConstantBuffers(CB_SLOT_CUSTOM, 1, &null_buffer);
	Render.ctx->PSSetConstantBuffers(CB_SLOT_CUSTOM, 1, &null_buffer);
	Render.ctx->CSSetConstantBuffers(CB_SLOT_CUSTOM, 1, &null_buffer);

	if (!cubemaps_vectors) {
		cubemaps_vectors = new CGPUBuffer();
		cubemaps_vectors->create(sizeof(VEC4), IR_SIZE * IR_SIZE * 6, false, "cubemaps_vecs");
		cubemaps_vectors->copyCPUtoGPUFrom(cubeMapVecs);
	}

	if (!probe_idx) {
		probe_idx = new CGPUBuffer();
		probe_idx->create(sizeof(uint) * 4, 1, false, "probe_idx");
	}

	struct sIrrData {
		uint probe_idx;
		uint cache_idx;
		uint dummy1;
		uint dummy2;
	};

	sIrrData irr_data = { 0, (uint)cache_idx, 0, 0 };
	probe_idx->copyCPUtoGPUFrom(&irr_data);

	CEntity* e_irr = spawn("data/prefabs/compute_irradiance.json", {});
	TCompCompute* c_compute = e_irr->get<TCompCompute>();

	//EngineRender.startRenderDocCapture();

	EngineCulling.setCamera(e_camera);
	EngineCullingShadows.setCamera(e_camera);

	int batch_idx = 0;
	// now compute the coeffs for every probe
	for (int i = 0; i < probes.size(); ++i)
	{
		const sProbe& p = probes[i];

		if (!p.disabled) {
			CGpuScope gpu_scope("Calculate Probe");

			// for every cubemap face
			for (int face = 0; face < 6; ++face)
			{
				CGpuScope gpu_scope("Render Face");

				int cubemap_idx = batch_idx * 6;
				render_texture->clear(Colors::Black, cubemap_idx + face);

				// compute camera orientation using defined vectors
				VEC3 eye = p.pos;
				VEC3 front = cubemapFaceNormals[face][2];
				VEC3 center = p.pos + front;
				VEC3 up = cubemapFaceNormals[face][1];
				c_camera->lookAt(eye, center, up);

				activateCamera(*c_camera);

				EngineCulling.run();
				EngineCullingShadows.run();

				// render the scene from this point of view
				CTexture::deactivate(TS_DEFERRED_OUTPUT);
				deferred_renderer.render(render_texture, e_camera, cubemap_idx + face);
			}
		}
		else {
			for (int face = 0; face < 6; ++face)
			{
				int cubemap_idx = batch_idx * 6;
				render_texture->clear(Colors::Black, cubemap_idx + face);
			}
		}

		batch_idx++;

		if (batch_idx < cubemap_batch_size && i != probes.size() - 1) {
			continue;
		}

		batch_idx = 0;

		CRenderToTexture::deactivate(4);

		sh_buffer->activateCSasUAV(0);
		cubemaps_vectors->activateCSasUAV(1);

		// Dispatch compute shader
		{
			CGpuScope gpu_scope("Calc Irradiance");

			probe_idx->activateCSasUAV(2);

			render_texture->activateCS(TS_ALBEDO);

			c_compute->update(Time.delta);

			render_texture->deactivateCS(TS_ALBEDO);
		}

		// Increase probe index
		irr_data.probe_idx += cubemap_batch_size;
		probe_idx->copyCPUtoGPUFrom(&irr_data);

		SendMessage(hwndPB, PBM_STEPIT, 0, 0);
	}


	CHandle h_mixed = getEntityByName("camera_mixed");
	EngineCulling.setCamera(h_mixed);
	EngineCullingShadows.setCamera(h_mixed);

	//EngineRender.stopRenderDocCapture();

	ID3D11UnorderedAccessView* nullUAV = nullptr;
	Render.ctx->CSSetUnorderedAccessViews(0, 1, &nullUAV, nullptr);
	Render.ctx->CSSetUnorderedAccessViews(1, 1, &nullUAV, nullptr);
	Render.ctx->CSSetUnorderedAccessViews(2, 1, &nullUAV, nullptr);
	Render.ctx->CSSetUnorderedAccessViews(3, 1, &nullUAV, nullptr);
	ID3D11Buffer* nullCB = nullptr;
	Render.ctx->CSSetConstantBuffers(CB_SLOT_WORLD, 1, &nullCB);

	static_cast<CHandle>(e_irr).destroy();

	Render.activateBackBuffer();
	sh_buffer->activatePS(TS_SH_0 + cache_idx);

	saveToFile(sh_buffer);

	setRenderingIrradianceEnabled(false);

	DestroyWindow(hwndPB);
}

void TCompIrradianceCache::renderProbes()
{
	if (!GLOBAL_RENDER_PROBES)
		return;

	CGpuScope gpu_scope("renderProbes");

	auto mesh = Resources.get("data/meshes/AreaSphere.mesh")->as<CMesh>();
	mesh->activate();

	auto material = Resources.get("data/materials/sh_texture.mat")->as<CMaterial>();
	material->activate();

	EngineFluidSimulation.activateFluids();

	for (auto& p : probes) {

		if (p.disabled) continue;

		activateObject(MAT44::CreateScale(0.12f) * MAT44::CreateTranslation(p.pos));

		mesh->render();
	}

	EngineFluidSimulation.deactivateFluids();
}

bool TCompIrradianceCache::saveToFile(CGPUBuffer* sh_buffer)
{
	CFileDataSaver fds(irradianceFilename.c_str());
	if (!fds.isValid())
		return false;

	sIrrHeader header;

	header.start = cte_world.sh_data[cache_idx].sh_grid_start;
	header.end = cte_world.sh_data[cache_idx].sh_grid_end;
	header.dims = cte_world.sh_data[cache_idx].sh_grid_dimensions;
	header.delta = cte_world.sh_data[cache_idx].sh_grid_delta;
	header.num_probes = (int)probes.size();

	sh_buffer->copyGPUtoCPU();

	fds.write(header);
	fds.writeBytes(sh_buffer->cpu_data.size(), sh_buffer->cpu_data.data());
	return fds.close();
}

bool TCompIrradianceCache::loadFromFile(CGPUBuffer* sh_buffer)
{
	sIrrHeader header;

	CFileDataProvider fdp(irradianceFilename.c_str());

	if (!fdp.isValid())
		return false;

	fdp.read(header);

	// Do not load if the cache needs to be recalculated
	if (grid_density != header.dims || start_pos != header.start || end_pos != header.end || delta != header.delta) {
		return false;
	}

	// Fill probes info
	fdp.readBytes(sh_buffer->cpu_data.size(), sh_buffer->cpu_data.data());
	sh_buffer->copyCPUtoGPU();

	sh_buffer->activatePS(TS_SH_0 + cache_idx);

	return fdp.close();
}
