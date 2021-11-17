#include "mcv_platform.h"
#include "comp_godrays.h"
#include "render/textures/render_to_texture.h"
#include "comp_fx_stack.h"
#include "render/blur_step.h"
#include "render/draw_primitives.h"
#include "components/common/comp_light_spot.h"

DECL_OBJ_MANAGER("god_rays", TCompGodRays);

void TCompGodRays::load(const json& j, TEntityParseContext& ctx)
{
	TCompBilateralBlur::load(j, ctx);

	idx = j.value("priority", idx);
	
	float width = Render.getWidth() * 0.5f;
	float height = Render.getHeight() * 0.5f;

	rt_volumetric_light = new CRenderToTexture();
	char rt_name[64];
	sprintf(rt_name, "VolumetricLightShape_%08x", CHandle(this).asUnsigned());
	bool is_ok = rt_volumetric_light->createRT(rt_name, static_cast<int>(width), 
		static_cast<int>(height), DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_UNKNOWN);
	assert(is_ok);

	// set apply callback
	apply_fn = std::bind(&TCompGodRays::generateVolumetricShape, this, std::placeholders::_1, std::placeholders::_2);
}

void TCompGodRays::onEntityCreated()
{
	TMsgFXToStack msg;
	msg.fx = CHandle(this);
	msg.priority = idx;
	CHandle(this).getOwner().sendMsg(msg);
}

void TCompGodRays::debugInMenu()
{
	TCompBilateralBlur::debugInMenu();

	getObjectManager<TCompLightSpot>()->forEach([&](TCompLightSpot* light_spot) {

		if (!light_spot->cast_godrays)
			return;

		CEntity* e = CHandle(light_spot).getOwner();
		if (ImGui::TreeNode(e->getName())) {
			light_spot->debugInMenu();
			ImGui::TreePop();
		}
	});
}

CTexture* TCompGodRays::generateVolumetricShape(CTexture* in_texture, CTexture* last_texture)
{
	if (!enabled)
		return in_texture;

	rt_volumetric_light->clear(VEC4::Zero);

	// Filter input image to gather only the highlights
	auto prev_rt = rt_volumetric_light->activateRT();
	assert(prev_rt);

	int rays = 0;

	getObjectManager<TCompLightSpot>()->forEach([&](TCompLightSpot* light_spot) {

		if (!light_spot->cast_godrays || !light_spot->activate())
			return;

		drawFullScreenQuad("godrays_shape.pipeline", in_texture);
		++rays;
	});

	if (!rays) {
		prev_rt->activateRT();
		return in_texture;
	}

	CTexture* blurred = TCompBilateralBlur::apply(rt_volumetric_light, last_texture);
	prev_rt->activateRT();
	drawFullScreenQuad("godrays_add.pipeline", rt_volumetric_light, blurred);

	return prev_rt;
}

void TCompGodRays::onRender(const TMsgRenderPostFX& msg)
{
	// don't render if it's not my turn
	if (CHandle(this) != msg.fx)
		return;

	TCompFXStack::updateRT(apply_fn);
}