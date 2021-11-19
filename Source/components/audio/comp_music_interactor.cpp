#include "mcv_platform.h"
#include "engine.h"
#include "components/audio/comp_music_interactor.h"
#include "components/controllers/comp_ai_controller_base.h"
#include "components/common/comp_transform.h"
#include "components/stats/comp_health.h"
#include "audio/module_audio.h"

DECL_OBJ_MANAGER("music_interactor", TCompMusicInteractor)

void TCompMusicInteractor::load(const json& j, TEntityParseContext& ctx)
{
	float min_monk_distance = j.value("min_monk_distance", 8.f);
	min_monk_distance_SQ = powf(min_monk_distance, 2.f);

	min_HP = j.value("min_HP", min_HP);
}

void TCompMusicInteractor::onEntityCreated()
{
	// Cache frequent components (used in update)
	h_cache_transform = get<TCompTransform>();
	h_cache_health = get<TCompHealth>();
}

void TCompMusicInteractor::update(float dt)
{
	if (!enabled)
		return;

	//// CHECKS TO ENABLE/DISABLE PROXIMITY LAYER ////
	float min_dist = min_monk_distance_SQ + 1.f;

	TCompTransform* h_trans = h_cache_transform;
	const VEC3 eon_pos = h_trans->getPosition();

	// Retrieve closest enemy distance
	getEntitiesByString("Enemy", [eon_pos, &min_dist](CHandle h) {
		// TODO: optimise if necessary, we only need to know if there is ONE or more enemies nearby
		CEntity* e = h;
		TCompAIControllerBase* h_ai = e->get<TCompAIControllerBase>();

		if (h_ai == nullptr)
			return false;

		// Keep closest distance inside min_dist
		CEntity* enemy = h;
		float dist = VEC3::DistanceSquared(eon_pos, enemy->getPosition());

		if (dist < min_dist)
			min_dist = dist;

		return true;
		});

	// If closest enemy is under the threshold, enable layer; else, disable
	if (min_dist < min_monk_distance_SQ) {
		EngineAudio.setMusicRTPC("Monks_Nearby", 1.f);
	}
	else {
		EngineAudio.setMusicRTPC("Monks_Nearby", 0.f);
	}
	/////////////////////////////////////////////////////

	// If low-hp, enable low-hp layer, else disable
	TCompHealth* h_health = h_cache_health;
	if (h_health->getHealthPercentage() <= min_HP) {
		EngineAudio.setMusicRTPC("Low_HP", 1.f);
	}
	else {
		EngineAudio.setMusicRTPC("Low_HP", 0.f);
	}
}

void TCompMusicInteractor::debugInMenu()
{
	ImGui::Checkbox("Enabled", &enabled);
	ImGui::DragInt("Target Count", &targeted_count);
	ImGui::DragFloat("Min Monk SQ_Distance for Battle-layer", &min_monk_distance_SQ);
	ImGui::DragFloat("Min HP for HP-layer", &min_HP);
}

void TCompMusicInteractor::onTargeted(const TMsgTarget& msg)
{
	++targeted_count;
	
	// Enable battle music layer
	EngineAudio.setMusicRTPC("Monk_Fight", 1.f);
}

void TCompMusicInteractor::onUntargeted(const TMsgUntarget& msg)
{
	if (targeted_count <= 0)
		return;

	--targeted_count;

	// If target_count is 0, disable battle music layer
	if (targeted_count <= 0)
		EngineAudio.setMusicRTPC("Monk_Fight", 0.f);
}

