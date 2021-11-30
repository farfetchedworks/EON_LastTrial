#include "mcv_platform.h"
#include "engine.h"
#include "comp_geons_manager.h"
#include "entity/entity.h"
#include "render/draw_primitives.h"
#include "components/stats/comp_attributes.h"
#include "components/common/comp_transform.h"
#include "lua/module_scripting.h"
#include "audio/module_audio.h"
#include "ui/ui_module.h"
#include "ui/widgets/ui_image.h"

DECL_OBJ_MANAGER("geons_manager", TCompGeonsManager)

void TCompGeonsManager::load(const json& j, TEntityParseContext& ctx)
{
	total_phases = j.value("total_phases", total_phases);
	first_phase_req = j.value("first_phase_req", first_phase_req);
	increase_ratio = j.value("increase_ratio", increase_ratio);
	
	// Load phase requirements
	phase_requirements[0] = first_phase_req;
	phase_requirements[1] = (int)(first_phase_req * increase_ratio);

	for (int i = 2; i < total_phases; ++i) {
		// Geons To Phase N = [(Geons To Phase N − 1) + (Geons To Phase N − 2)] * Geons Increase Ratio
		phase_requirements[i] = (int)((phase_requirements[i - 1] + phase_requirements[i - 2]) * increase_ratio);
	}

	ui::CWidget* w = EngineUI.getWidgetFrom("eon_hud", "geons_bar");
	assert(w);
	ui::CWidget* wChild = w->getChildByName("bar_fill");
	if (wChild) {
		ui::CImage* fill = static_cast<ui::CImage*>(wChild);
		ui::TImageParams& params = fill->imageParams;
		params.alpha_cut = 0.f;
	}
}

void TCompGeonsManager::update(float dt)
{
	lerpGeons(dt);

	if (timer > 0.f)
	{
		timer -= dt;
		if (timer <= 0.f)
		{
			EngineUI.deactivateWidget("phase_up");
		}
	}

	// Update UI
	int current_req = phase_requirements[prev_phase + 1];
	float pct = lerp_geons / (float)current_req;
	
	ui::CWidget* w = EngineUI.getWidgetFrom("eon_hud", "geons_bar");
	assert(w);
	ui::CWidget* wChild = w->getChildByName("bar_fill");
	if (wChild) {
		ui::CImage* fill = static_cast<ui::CImage*>(wChild);
		ui::TImageParams& params = fill->imageParams;
		params.alpha_cut = pct;
	}

	prev_phase = (prev_phase < phase && pct >= 0.99f) ? ++prev_phase : prev_phase;
	lerp_geons = pct < 0.001f ? 0.f : lerp_geons;
}

void TCompGeonsManager::addGeons(int geons)
{
	current_geons += geons;
	int current_phase = phase;

	// Check new phase (can increment >1 phases in one shot?)
	for (int i_phase = current_phase; i_phase < total_phases; ++i_phase) {
		
		int current_req = phase_requirements[i_phase + 1];

		if (current_geons >= current_req) {
			// Geons After PhaseUp = (CurrentGeons + GainedGeons) − PhaseRequiredGeons
			current_geons = (current_geons - current_req);
			increasePhase();
		}
		else
		{
			// No more checking
			break;
		}
	}
}

void TCompGeonsManager::lerpGeons(float dt)
{
	int current_req = phase_requirements[prev_phase + 1];

	if (prev_phase < phase)
		lerp_geons = damp<float>(lerp_geons, (float) current_req, 2.5f, dt);
	else
		lerp_geons = damp<float>(lerp_geons, (float) current_geons, 2.5f, dt);
}

void TCompGeonsManager::increasePhase(bool only_stats)
{
	if (phase >= 10)
		return;

	++phase;

	TCompAttributes* attrs = get<TCompAttributes>();
	attrs->onNewPhase(phase);

	// Update UI
	ui::CWidget* w = EngineUI.getWidget("phase_number");
	assert(w);
	ui::CImage* img = static_cast<ui::CImage*>(w);
	ui::TImageParams& params = img->imageParams;
	std::string path = "data/textures/ui/subvert/HUD/PhaseNumbers/" + std::to_string(phase) + ".dds";
	params.texture = Resources.get(path)->as<CTexture>();
	assert(params.texture);

	if (!only_stats)
	{
		TCompTransform* c_trans = get<TCompTransform>();
		spawnParticles("data/particles/compute_levelup_smoke_particles.json", c_trans->getPosition(), c_trans->getPosition());
		spawnParticles("data/particles/compute_levelup_spread_particles.json", c_trans->getPosition(), c_trans->getPosition());
		EngineUI.activateWidget("phase_up");
		timer = 3.f;

		// FMOD audio event
		EngineAudio.postEvent("UI/Phase_Up");
	}
}

void TCompGeonsManager::setPhase(int new_phase)
{
	for (int i = 0; i < (new_phase - 1); ++i)
		increasePhase(true);
}

void TCompGeonsManager::onEnemyDied(const TMsgEnemyDied& msg)
{
	int geonsGained = msg.geons;
	//if (msg.backstabDeath) geonsGained *= 2;			// This line is commented because a backstab death will not currently increase the amount of gained geons. Pending to be analyzed by balancing
	
	// Don't do this directly
	// addGeons(geonsGained);

	// Alex: 3sec is the aprox time to add geons when particles arrive!
	EngineLua.executeScript("addGeons(" + std::to_string(geonsGained) + ")", 3.f);
}

void TCompGeonsManager::debugInMenu()
{
	int current_req = phase_requirements[phase + 1];
	float pct = current_geons / (float)current_req;
	ImGui::Text("Geons: %i, Current Req: %d, Fill: %f", current_geons, current_req, pct);

	ImGui::Separator();
	for (int i = 2; i < total_phases; ++i) {
		ImGui::Text("To Phase %d: %d", i, phase_requirements[i]);
	}
	ImGui::Separator();
	if (ImGui::Button("DBG: Add 150"))
		addGeons(34);
}

void TCompGeonsManager::renderDebug()
{
	/*std::string ss = "Geons: " + std::to_string(current_geons) + " (PHASE " + std::to_string(phase) + ")";
	drawText2D(VEC2(35, 90), Colors::White, ss.c_str());*/
}

