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

const int MAX_HUD_WARP = 9;

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

void TCompWarpEnergy::onEntityCreated()
{
	for (int i = 1; i <= MAX_HUD_WARP; ++i)
	{
		// Update UI
		ui::CWidget* w = EngineUI.getWidgetFrom("eon_hud", "warp_energy_bar");
		assert(w);
		std::string slotName = "bar_fill_slot_" + std::to_string(i);
		ui::CWidget* wChild = w->getChildByName(slotName);
		if(!wChild)
			continue;
		ui::CImage* fill = static_cast<ui::CImage*>(wChild);
		ui::TImageParams& params = fill->imageParams;
		params.alpha_cut = 0.f;
	}
}

void TCompWarpEnergy::update(float dt)
{
#ifdef AUTO_WARP_SLOT
	if (warp_energy < 1.f) {
		warp_energy += dt * warp_recovery_speed;
		warp_energy = std::clamp(warp_energy, 0.0f, 1.0f);
	}
#endif // AUTO_WARP_SLOT

	for (int i = 1; i <= curr_max_warp_energy; ++i)
	{
		// Update UI
		ui::CWidget* w = EngineUI.getWidgetFrom("eon_hud", "warp_energy_bar");
		assert(w);
		std::string slotName = "bar_fill_slot_" + std::to_string(i);
		ui::CWidget* wChild = w->getChildByName(slotName);
		assert(wChild);
		ui::CImage* fill = static_cast<ui::CImage*>(wChild);
		// Compute pct for this slot
		ui::TImageParams& params = fill->imageParams;
		params.alpha_cut = clampf(1.f + warp_energy - (float)i, 0.f, 1.f);
	}

	if (empty_warp_timer < 0.f)
		return;
	
	empty_warp_timer -= dt;

	if (empty_warp_timer < 0.f)
	{
		ui::CWidget* w = EngineUI.getWidgetFrom("eon_hud", "warp_energy_bar");
		assert(w);
		ui::CWidget* wChild = w->getChildByName("bar_background_complete");
		assert(wChild);
		ui::CImage* img = static_cast<ui::CImage*>(wChild);
		ui::TImageParams& params = img->imageParams;
		params.texture = Resources.get("data/textures/ui/subvert/HUD/WarpEnergyMarco.dds")->as<CTexture>();
	}
}

void TCompWarpEnergy::addSlot()
{
	if (curr_max_warp_energy == MAX_HUD_WARP)
		return;

	curr_max_warp_energy++;

	int extraSlots = curr_max_warp_energy - 6; // 6 is the base slots
	for (int i = 0; i < extraSlots; ++i)
	{
		// Update UI
		ui::CWidget* w = EngineUI.getWidgetFrom("eon_hud", "warp_energy_bar");
		assert(w);
		std::string slotName = "bar_extra_slot_" + std::to_string(i + 1);
		ui::CWidget* wChild = w->getChildByName(slotName);
		assert(wChild);
		ui::CImage* img = static_cast<ui::CImage*>(wChild);
		img->setVisible(true);
	}
}

void TCompWarpEnergy::debugInMenu()
{
	ImGui::DragFloat("Warp", &warp_energy, 0.1f, 0.f, (float)curr_max_warp_energy);

	if (ImGui::Button("Fill"))
		fillWarpEnergy();
	if (ImGui::Button("+1 Max"))
		addSlot();
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
		ui::CWidget* wChild = w->getChildByName("bar_background_complete");
		assert(wChild);
		ui::CImage* img = static_cast<ui::CImage*>(wChild);
		ui::TImageParams& params = img->imageParams;
		params.texture = Resources.get("data/textures/ui/subvert/HUD/WarpEnergyMarcoEmpty.dds")->as<CTexture>();
		empty_warp_timer = 0.5f;
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
