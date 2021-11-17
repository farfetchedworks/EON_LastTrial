#include "mcv_platform.h"
#include "comp_render_reflections.h"
#include "resources/resources.h"
#include "components/common/comp_camera.h"
#include "render/textures/render_to_texture.h"
#include "render/draw_primitives.h"
#include "comp_fx_stack.h"

DECL_OBJ_MANAGER("ssreflections", TCompSSReflections);

void TCompSSReflections::load(const json& j, TEntityParseContext& ctx) {

	TCompBilateralBlur::load(j, ctx);

	enabled = j.value("enabled", enabled);
	idx = j.value("priority", idx);

	float width = Render.getWidth() * 0.5f;
	float height = Render.getHeight() * 0.5f;

	rt_output = new CRenderToTexture;
	char rt_name[64];
	sprintf(rt_name, "SSR_%08x", CHandle(this).asUnsigned());
	bool is_ok = rt_output->createRT(rt_name, static_cast<int>(width),
		static_cast<int>(height), DXGI_FORMAT_R16G16B16A16_FLOAT);
	assert(is_ok);

	// set apply callback
	apply_fn = std::bind(&TCompSSReflections::compute, this, std::placeholders::_1, std::placeholders::_2);
}

void TCompSSReflections::onEntityCreated()
{
	TMsgFXToStack msg;
	msg.fx = CHandle(this);
	msg.priority = idx;
	CHandle(this).getOwner().sendMsg(msg);
}

void TCompSSReflections::debugInMenu() {

	TCompBilateralBlur::debugInMenu();

	ImGui::Checkbox("SSR Enabled", &enabled);
}

CTexture* TCompSSReflections::compute(CTexture* in_texture, CTexture* last_texture)
{
	if (!enabled)
		return in_texture;

	assert(in_texture);

	CGpuScope gpu_scope("SSR");

	auto prev_rt = rt_output->activateRT();

	// draw reflections
	drawFullScreenQuad("ssr.pipeline", in_texture);

	// Stop rendering into the rt_output
	CTexture::deactivate(0);

	CTexture* blurred = TCompBilateralBlur::apply(rt_output, last_texture);

	prev_rt->activateRT();
	// add to final rt
	drawFullScreenQuad("ssr_add.pipeline", blurred);

	return prev_rt;
}

void TCompSSReflections::onRender(const TMsgRenderPostFX& msg)
{
	// don't render if it's not my turn
	if (CHandle(this) != msg.fx)
		return;

	TCompFXStack::updateRT(apply_fn);
}