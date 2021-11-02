#include "mcv_platform.h"
#include "engine.h"
#include "comp_stamina.h"
#include "components/common/comp_transform.h"
#include "components/common/comp_name.h"
#include "render/draw_primitives.h"
#include "ui/ui_module.h"
#include "ui/widgets/ui_image.h"
#include <mmsystem.h>

DECL_OBJ_MANAGER("stamina", TCompStamina)

static NamedValues<EAction>::TEntry output_entries[] = {
  {EAction::ATTACK_REGULAR, "regular_attack"},
  {EAction::ATTACK_REGULAR_COMBO, "combo_attack"},
  {EAction::ATTACK_REGULAR_SPRINT, "regular_sprint_attack"},
  {EAction::ATTACK_HEAVY, "strong_attack"},
  {EAction::ATTACK_HEAVY_CHARGE, "charge_attack"},
  {EAction::ATTACK_HEAVY_SPRINT, "strong_sprint_attack"},
  {EAction::SPRINT, "sprint"},
  {EAction::DASH, "dash"},
  {EAction::DASH_STRIKE, "dash_strike"},
  {EAction::PARRY, "parry"}
};
static NamedValues<EAction> output_names(output_entries, sizeof(output_entries) / sizeof(NamedValues<EAction>::TEntry));

void TCompStamina::load(const json& j, TEntityParseContext& ctx)
{
	max_stamina = j.value("max_stamina", max_stamina);
	recovery_frequency = j.value("recovery_frequency", recovery_frequency);
	recovery_points = j.value("recovery_points", recovery_points);
	recovery_penalization = j.value("recovery_penalization", recovery_penalization);
	recovery_penalization_at_0 = j.value("recovery_penalization_at_0", recovery_penalization_at_0);

	assert(j.count("costs"));
	assert(j["costs"].is_object());
	
	for (auto e : j["costs"].items()) {
		const std::string& name = e.key();
		stamina_costs[output_names.valueOf(name.c_str())] = { e.value(), name };
	}
}

void TCompStamina::update(float dt)
{
	// Update UI
	float pct = current_stamina / max_stamina;
	ui::CWidget* w = EngineUI.getWidgetFrom("eon_hud", "stamina_bar");
	assert(w);
	ui::CWidget* wChild = w->getChildByName("bar_fill");
	if (wChild) {
		ui::CImage* fill = static_cast<ui::CImage*>(wChild);
		ui::TImageParams& params = fill->imageParams;
		params.alpha_cut = pct;
	}
}

void TCompStamina::fillStamina()
{
	current_stamina = max_stamina;
}

void TCompStamina::reduceStamina(EAction player_action)
{
	TCompPlayerController* controller = get<TCompPlayerController>();

	if (controller && controller->GOD_MODE)
		return;

	current_stamina = std::max<float>(current_stamina - stamina_costs[player_action].stamina_cost, 0);
	time_elapsed = 0.f;

	// dbg("REDUCED %d STAMINA\n", stamina_costs[player_action].stamina_cost);

	if (current_stamina == 0.f) {
		recovery_penalized = true;
	}
}

bool TCompStamina::hasMaxStamina()
{
	return (current_stamina == max_stamina);
}

bool TCompStamina::hasStamina(EAction player_action)
{
	return (current_stamina >= stamina_costs[player_action].stamina_cost);
}

bool TCompStamina::hasStamina()
{
	return current_stamina > 0.f;
}

void TCompStamina::recoverStamina(bool can_recover, float dt)
{
	if (!can_recover) {
		last_can_recover = can_recover;
		return;
	}

	time_elapsed += dt;

	// In case we were blocked, wait some time to begin recovering again
	if (last_can_recover != can_recover) {
		
		if (time_elapsed >= recovery_penalization) {
			last_can_recover = can_recover;
			time_elapsed = 0.f;
		}

		return;
	}

	if (recovery_penalized) {

		if (time_elapsed >= recovery_penalization_at_0) {
			recovery_penalized = false;
			time_elapsed = 0.f;
		}

		return;
	}

	if (time_elapsed >= recovery_frequency && current_stamina != max_stamina) {
		current_stamina = std::min<float>(current_stamina + recovery_points, max_stamina);
		time_elapsed = 0;
	}
}

void TCompStamina::debugInMenu()
{
	// Show current stamina first
	ImGui::PushStyleColor(ImGuiCol_PlotHistogram, (ImVec4)ImColor::ImColor(0.f, 0.5f, 0.5f));
	ImGui::ProgressBar(current_stamina / max_stamina, ImVec2(-1, 0));
	ImGui::PopStyleColor();

	/*ImGui::PushStyleColor(ImGuiCol_PlotHistogram, (ImVec4)ImColor::ImColor(0.11f, 0.8f, 0.15f));
	ImGui::ProgressBar(health / 100.f, ImVec2(-1, 0));
	ImGui::PopStyleColor();*/

	ImGui::DragFloat("Max Stamina", &max_stamina, 10.f, 50.f, 200.f);
	ImGui::Separator();

	ImGui::Columns(2, "mycolumns"); // 4-ways, with border
	ImGui::Separator();
	ImGui::Text("Action"); ImGui::NextColumn();
	ImGui::Text("Cost"); ImGui::NextColumn();
	ImGui::Separator();
	ImGui::Columns(1);

	if (ImGui::BeginTable("Stamina costs", 2))
	{
		for (auto it = stamina_costs.begin(); it != stamina_costs.end(); it++)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text(it->second.action_name.c_str());
			ImGui::TableNextColumn();
			ImGui::Text("%d", it->second.stamina_cost);
		}
		ImGui::EndTable();
	}
}

void TCompStamina::renderDebug() {

	TCompName* c_name = get<TCompName>();
	if (!strcmp(c_name->getName(), "player")) {

		// drawProgressBar2D(VEC2(70, 50), Colors::Yellow, current_stamina, max_stamina, "stamina");
	}
	else
	{
		TCompTransform* trans = get<TCompTransform>();
		VEC3 pos = trans->getPosition();
		drawProgressBar3D(pos, Colors::Yellow, current_stamina, max_stamina, VEC2(0.f, 10.f));
	}
}

float TCompStamina::getCurrentStamina()
{
	return current_stamina;
}
