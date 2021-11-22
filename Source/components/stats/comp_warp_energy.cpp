#include "mcv_platform.h"
#include "engine.h"
#include "comp_warp_energy.h"
#include "entity/entity.h"
#include "render/draw_primitives.h"
#include "components/messages.h"
#include "components/common/comp_transform.h"
#include "components/common/comp_name.h"
#include "components/controllers/comp_player_controller.h"
#include "ui/ui_module.h"
#include "ui/widgets/ui_image.h"

// #define AUTO_WARP_SLOT

DECL_OBJ_MANAGER("warp_energy", TCompWarpEnergy)

void TCompWarpEnergy::load(const json& j, TEntityParseContext& ctx)
{
	assert(j["max_warp_energy"].is_number());
	assert(j["on_hit_amount_warp"].is_number());

	max_warp_energy = j.value("max_warp_energy", max_warp_energy);
	curr_max_warp_energy = j.value("curr_max_warp_energy", max_warp_energy);
	on_hit_amount_warp = j.value("on_hit_amount_warp", on_hit_amount_warp);
	warp_recovery_speed = j.value("warp_recovery_speed", warp_recovery_speed);
	
	fillWarpEnergy();
}

void TCompWarpEnergy::update(float dt)
{
#ifdef AUTO_WARP_SLOT
	if (warp_energy < 1.f) {
		warp_energy += dt * warp_recovery_speed;
		warp_energy = std::clamp(warp_energy, 0.0f, 1.0f);
	}
#endif // AUTO_WARP_SLOT

	// Update UI
	float pct = warp_energy / (float)max_warp_energy;
	ui::CWidget* w = EngineUI.getWidgetFrom("eon_hud", "warp_energy_bar");
	assert(w);
	ui::CWidget* wChild = w->getChildByName("bar_fill");
	if (wChild) {
		ui::CImage* fill = static_cast<ui::CImage*>(wChild);
		ui::TImageParams& params = fill->imageParams;
		params.alpha_cut = pct;
	}

	pct = (warp_energy - max_warp_energy) / (float)max_warp_energy;
	ui::CWidget* w_1 = EngineUI.getWidgetFrom("eon_hud", "warp_energy_bar_1");
	assert(w_1);
	wChild = w_1->getChildByName("bar_fill");
	if (wChild) {
		ui::CImage* fill = static_cast<ui::CImage*>(wChild);
		ui::TImageParams& params = fill->imageParams;
		params.alpha_cut = pct;
	}

	/*wChild = w_1->getChildByName("bar_background_all");
	if (wChild) {
		ui::CImage* fill = static_cast<ui::CImage*>(wChild);
		ui::TImageParams& params = fill->imageParams;
		params.alpha_cut = 1 - pct;
	}*/

	if (empty_warp_timer > 0.f)
	{
		empty_warp_timer -= dt;

		if (empty_warp_timer < 0.f)
		{
			ui::CWidget* wChild = w->getChildByName("bar_background_empty");
			assert(wChild);
			ui::CImage* img = static_cast<ui::CImage*>(wChild);
			img->setVisible(false);

			wChild = w_1->getChildByName("bar_background_empty");
			assert(wChild);
			img = static_cast<ui::CImage*>(wChild);
			img->setVisible(false);
		}
	}
}

void TCompWarpEnergy::debugInMenu()
{
	ImGui::PushStyleColor(ImGuiCol_PlotHistogram, (ImVec4)ImColor::ImColor(0.11f, 0.15f, 0.8f));
	ImGui::ProgressBar(warp_energy / (float)curr_max_warp_energy, ImVec2(-1, 0));
	ImGui::PopStyleColor();

	if (ImGui::Button("DBG: Fill"))
		fillWarpEnergy();
}

void TCompWarpEnergy::onHit(const TMsgHitWarpRecover& msg)
{
	if (!msg.hitByPlayer)
		return;

	float multiplier = msg.multiplier;

	if (msg.fromBack) {
		multiplier *= 2.f;
	}

	warp_energy = std::min<float>(warp_energy +
		on_hit_amount_warp * multiplier, (float)curr_max_warp_energy);
}

bool TCompWarpEnergy::hasWarpEnergy(int warp_cost)
{ 
	bool is_ok = floor(warp_energy) >= warp_cost;

	if (!is_ok)
	{
		ui::CWidget* w = EngineUI.getWidgetFrom("eon_hud", "warp_energy_bar");
		assert(w);
		ui::CWidget* wChild = w->getChildByName("bar_background_empty");
		if (wChild) {
			ui::CImage* img = static_cast<ui::CImage*>(wChild);
			img->setVisible(true);
			empty_warp_timer = 0.5f;
		}

		w = EngineUI.getWidgetFrom("eon_hud", "warp_energy_bar_1");
		assert(w);
		wChild = w->getChildByName("bar_background_empty");
		if (wChild) {
			ui::CImage* img = static_cast<ui::CImage*>(wChild);
			img->setVisible(true);
			empty_warp_timer = 0.5f;

			float pct = (curr_max_warp_energy - max_warp_energy) / (float)max_warp_energy;
			ui::CImage* fill = static_cast<ui::CImage*>(wChild);
			ui::TImageParams& params = fill->imageParams;
			params.alpha_cut = pct;
		}
	}

	return is_ok;
}

void TCompWarpEnergy::consumeWarpEnergy(int warp_nrg_points)
{
	CEntity* owner = getEntity();
	TCompPlayerController* pawn = owner->get<TCompPlayerController>();
	assert(pawn);
	if(!pawn->GOD_MODE)
		warp_energy = std::max<float>(warp_energy - static_cast<float>(warp_nrg_points), 0.0f);
}

void TCompWarpEnergy::renderDebug() {
	
	TCompName* c_name = get<TCompName>();
	if (!strcmp(c_name->getName(), "player")) {
		/*float slot_width = 20;
		for (int i = 0; i < max_warp_energy; ++i) {
			VEC2 pos = VEC2(70 + i * slot_width, 30);
			float val = std::clamp(warp_energy, 0.f, i + 1.f);
			val -= (warp_energy >= i ? i : warp_energy);
			drawProgressBar2D(pos, Colors::ShinyBlue, val, 1.f, nullptr, slot_width);
		}*/
	}
	else
	{
		TCompTransform* trans = get<TCompTransform>();
		VEC3 pos = trans->getPosition();
		drawProgressBar3D(pos, Colors::Blue, warp_energy, static_cast<float>(curr_max_warp_energy), VEC2(0.f, 20.f));
	}
}
