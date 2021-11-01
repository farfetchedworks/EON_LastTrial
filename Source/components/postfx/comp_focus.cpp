#include "mcv_platform.h"
#include "comp_focus.h"
#include "engine.h"
#include "render/textures/render_to_texture.h"
#include "comp_fx_stack.h"
#include "render/render_module.h"

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
	enabled = j.value("enabled", enabled);
	cte_focus.focus_z_center_in_focus = j.value("z_center_in_focus", 30.f);
	cte_focus.focus_z_margin_in_focus = j.value("z_margin_in_focus", 0.f);
	cte_focus.focus_transition_distance = j.value("transition_distance", 15.f);
	cte_focus.focus_modifier = j.value("focus_modifier", 1.f);
	pipeline = Resources.get("focus.pipeline")->as<CPipelineState>();
	mesh = Resources.get("unit_quad_xy.mesh")->as<CMesh>();

	idx = j.value("priority", idx);

	// with the first use, init with the input resolution
	rt = new CRenderToTexture;
	// Make it HDR!!
	bool is_ok = rt->createRT("RT_Focus", Render.getWidth(), Render.getHeight(), DXGI_FORMAT_R8G8B8A8_UNORM);
	assert(is_ok);

	// set apply callback
	apply_fn = std::bind(&TCompFocus::apply, this, std::placeholders::_1, std::placeholders::_2);
}

void TCompFocus::debugInMenu()
{
	ImGui::Checkbox("Enabled", &enabled);
	ImGui::DragFloat("Z Center In Focus", &cte_focus.focus_z_center_in_focus, 0.1f, 0.f, 1000.f);
	ImGui::DragFloat("Margin In Focus", &cte_focus.focus_z_margin_in_focus, 0.1f, 0.f, 300.f);
	ImGui::DragFloat("Transition Distance", &cte_focus.focus_transition_distance, 0.1f, 0.f, 1000.f);
	ImGui::DragFloat("Focus Modifier", &cte_focus.focus_modifier, 0.1f, 0.f, 1.0f);
}

CTexture* TCompFocus::apply(CTexture* blur_texture, CTexture* last_texture)
{
	CTexture* focus_texture = EngineRender.getFinalRT();

	if (!enabled)
		return blur_texture;

	CGpuScope gpu_scope("CompFocus");

	cte_focus.updateFromCPU();
	cte_focus.activate();

	assert(mesh);
	assert(pipeline);
	assert(focus_texture);
	assert(blur_texture);

	rt->activateRT();
	focus_texture->activate(TS_ALBEDO);
	blur_texture->activate(TS_NORMAL);      // As long as matches the shader, and it's not ALBEDO
	pipeline->activate();
	mesh->activate();
	mesh->render();

	return rt;
}

void TCompFocus::onRender(const TMsgRenderPostFX& msg)
{
	// don't render if it's not my turn
	if (CHandle(this) != msg.fx)
		return;

	TCompFXStack::updateRT(apply_fn);
}