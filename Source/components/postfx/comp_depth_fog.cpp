#include "mcv_platform.h"
#include "comp_depth_fog.h"
#include "comp_fx_stack.h"
#include "render/textures/render_to_texture.h"
#include "render/draw_primitives.h"

DECL_OBJ_MANAGER("depth_fog", TCompDepthFog);

TCompDepthFog::TCompDepthFog()
{
	bool is_ok = ctes.create(CB_SLOT_FOCUS, "DepthFog");
	assert(is_ok);
}

TCompDepthFog::~TCompDepthFog()
{
	ctes.destroy();
}

void TCompDepthFog::load(const json& j, TEntityParseContext& ctx)
{
	enabled = j.value("enabled", enabled);
	idx = j.value("priority", idx);

	ctes.fog_color = loadVEC3(j, "color");
	ctes.fog_factor_decay = j.value("decay", 0.f);
	ctes.fog_factor_exponent = j.value("exponent", 0.f);
	ctes.updateFromCPU();

	rt_fog = new CRenderToTexture();
	char rt_name[64];
	sprintf(rt_name, "Fog_%08x", CHandle(this).asUnsigned());
	bool is_ok = rt_fog->createRT(rt_name, static_cast<int>(Render.getWidth()),
		static_cast<int>(Render.getHeight()), DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_UNKNOWN);
	assert(is_ok);

	// set apply callback
	apply_fn = std::bind(&TCompDepthFog::renderFog, this, std::placeholders::_1, std::placeholders::_2);
}

void TCompDepthFog::onEntityCreated()
{
	TMsgFXToStack msg;
	msg.fx = CHandle(this);
	msg.priority = idx;
	CHandle(this).getOwner().sendMsg(msg);
}

CTexture* TCompDepthFog::renderFog(CTexture* in_texture, CTexture* last_texture) {

	if (!enabled)
		return in_texture;

	ctes.activate();

	rt_fog->clear(VEC4::Zero);
	rt_fog->activateRT();
	drawFullScreenQuad("fog.pipeline", in_texture);

	return rt_fog;
}

void TCompDepthFog::debugInMenu() {
	ImGui::Checkbox("Enabled", &enabled);
	bool changed = ImGui::DragFloat("Fog Decay", &ctes.fog_factor_decay, 1.f, 0.f, 150.0f);
	changed |= ImGui::DragFloat("Fog Exponent", &ctes.fog_factor_exponent, 0.1f, 1.f, 10.0f);
	changed |= ImGui::ColorEdit4("Fog Color", &ctes.fog_color.x);

	if (changed) {
		ctes.updateFromCPU();
	}
}

void TCompDepthFog::onRender(const TMsgRenderPostFX& msg)
{
	// don't render if it's not my turn
	if (CHandle(this) != msg.fx)
		return;

	TCompFXStack::updateRT(apply_fn);
}