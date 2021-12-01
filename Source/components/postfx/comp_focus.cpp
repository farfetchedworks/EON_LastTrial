#include "mcv_platform.h"
#include "comp_focus.h"
#include "engine.h"
#include "render/textures/render_to_texture.h"
#include "comp_fx_stack.h"
#include "render/render_module.h"
#include "render/draw_primitives.h"

DECL_OBJ_MANAGER("focus", TCompFocus);

TCompFocus::TCompFocus()
{
	bool is_ok = cte_focus.create(CB_SLOT_FOCUS, "Focus");
	assert(is_ok);
}

TCompFocus::~TCompFocus()
{
	cte_focus.destroy();
}

void TCompFocus::onEntityCreated()
{
	TMsgFXToStack msg;
	msg.fx = CHandle(this);
	msg.priority = idx;
	CHandle(this).getOwner().sendMsg(msg);
}

void TCompFocus::load(const json& j, TEntityParseContext& ctx)
{
	TCompBilateralBlur::load(j, ctx);

	enabled = j.value("enabled", enabled);
	cte_focus.focus_z_center_in_focus = j.value("z_center_in_focus", 5.f);
	cte_focus.focus_z_margin_in_focus = j.value("z_margin_in_focus", 0.f);
	cte_focus.focus_transition_distance = j.value("transition_distance", 4.0f);
	cte_focus.focus_intensity = j.value("focus_intensity", 0.f);
	pipeline = Resources.get("focus.pipeline")->as<CPipelineState>();
	mesh = Resources.get("unit_quad_xy.mesh")->as<CMesh>();

	idx = j.value("priority", idx);

	// with the first use, init with the input resolution
	rt = new CRenderToTexture;
	// Make it HDR!!
	bool is_ok = rt->createRT("RT_Focus", Render.getWidth(), Render.getHeight(), DXGI_FORMAT_R16G16B16A16_FLOAT);
	assert(is_ok);

	// set apply callback
	apply_fn = std::bind(&TCompFocus::apply, this, std::placeholders::_1, std::placeholders::_2);
}

void TCompFocus::debugInMenu()
{
	TCompBilateralBlur::debugInMenu();

	ImGui::Checkbox("Focus Enabled", &enabled);
	ImGui::DragFloat("Z Center In Focus", &cte_focus.focus_z_center_in_focus, 0.1f, 0.f, 1000.f);
	ImGui::DragFloat("Margin In Focus", &cte_focus.focus_z_margin_in_focus, 0.1f, 0.f, 300.f);
	ImGui::DragFloat("Transition Distance", &cte_focus.focus_transition_distance, 0.1f, 0.f, 1000.f);
	ImGui::DragFloat("Focus Intensity", &cte_focus.focus_intensity, 0.1f, 0.f, 1.0f);
}

void TCompFocus::enable()
{
	TCompFocus::enabled = true;
	TCompBilateralBlur::enabled = true;
}

void TCompFocus::disable()
{
	TCompFocus::enabled = false;
	TCompBilateralBlur::enabled = false;
}

CShaderCte<CtesFocus>& TCompFocus::getCtesFocus()
{
	return cte_focus;
}

CTexture* TCompFocus::apply(CTexture* in_texture, CTexture* last_texture)
{
	if (!enabled)
		return in_texture;

	CGpuScope gpu_scope("CompFocus");

	rt->clear(VEC4::Zero);

	cte_focus.updateFromCPU();
	cte_focus.activate();

	CTexture* blurred = TCompBilateralBlur::apply(in_texture, last_texture);

	rt->activateRT();
	drawFullScreenQuad("focus.pipeline", in_texture, blurred);

	return rt;
}

void TCompFocus::onRender(const TMsgRenderPostFX& msg)
{
	// don't render if it's not my turn
	if (CHandle(this) != msg.fx)
		return;

	TCompFXStack::updateRT(apply_fn);
}