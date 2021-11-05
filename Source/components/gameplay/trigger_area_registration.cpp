#include "mcv_platform.h"
#include "handle/handle.h"
#include "engine.h"
#include "entity/entity.h"
#include "components/cameras/comp_camera_follow.h"
#include "components/gameplay/trigger_area.h"
#include "components/gameplay/comp_trigger_area.h"
#include "components/gameplay/comp_game_manager.h"
#include "components/ai/comp_bt.h"
#include "components/controllers/comp_player_controller.h"
#include "components/abilities/comp_time_reversal.h"
#include "components/stats/comp_health.h"
#include "components/common/comp_hierarchy.h"
#include "components/common/comp_collider.h"
#include "components/common/comp_tags.h"
#include "components/audio/comp_music_interactor.h"
#include "skeleton/comp_skeleton.h"
#include "lua/module_scripting.h"
#include "audio/module_audio.h"

#pragma region General Gameplay

class CTriggerAreaDeathArea : public ITriggerArea {

public:
	CTriggerAreaDeathArea(const std::string& name) : ITriggerArea(name) {}

	void onAreaEnter(CHandle event_trigger, CHandle observer) override
	{
		CEntity* e_trigger_area = event_trigger;
		TCompTriggerArea* c_trigger_area = e_trigger_area->get<TCompTriggerArea>();
		CEntity* e_object_hit = c_trigger_area->h_hit_object;
		e_object_hit->sendMsg(TMsgPawnDead());
	}
};

class CTriggerAreaShrineArea : public ITriggerArea {

public:
	CTriggerAreaShrineArea(const std::string& name) : ITriggerArea(name) {}

	void onAreaEnter(CHandle event_trigger, CHandle observer) override
	{
		CEntity* e_trigger_area = event_trigger;
		TCompHierarchy* c_hierarchy = e_trigger_area->get<TCompHierarchy>();

		if (c_hierarchy) {
			CEntity* e_entity_parent = c_hierarchy->getRootEntity(e_trigger_area);
			e_entity_parent->sendMsg(TMsgEnemyEnter());
		}
	}

	void onAreaExit(CHandle event_trigger, CHandle observer) override
	{
		CEntity* e_trigger_area = event_trigger;
		TCompHierarchy* c_hierarchy = e_trigger_area->get<TCompHierarchy>();

		if (c_hierarchy) {
			CEntity* e_entity_parent = c_hierarchy->getRootEntity(e_trigger_area);;
			e_entity_parent->sendMsg(TMsgEnemyExit());
		}
	}
};

class CTriggerAreaClearTRBuffer : public ITriggerArea {

public:
	CTriggerAreaClearTRBuffer(const std::string& name) : ITriggerArea(name) {}

	void onAreaEnter(CHandle event_trigger, CHandle observer) override
	{
		CEntity* player = getEntityByName("player");
		CEntity* trigger = getAreaTrigger(event_trigger);

		if (player != trigger)
			return;

		TCompTimeReversal* c_time_reversal = player->get<TCompTimeReversal>();
		assert(c_time_reversal);
		c_time_reversal->clearBuffer();
	}
};

#pragma endregion

#pragma region Boss Arenas
class CTriggerAreaGardArena : public ITriggerArea {
	
public: 
	CTriggerAreaGardArena(const std::string& name) : ITriggerArea(name) {}

	void onAreaEnter(CHandle event_trigger, CHandle observer) override
	{
		CEntity* e_gard = getEntityByName("Gard");
		assert(e_gard);
		TCompBT* c_bt_gard = e_gard->get<TCompBT>();
		assert(c_bt_gard);

		if (c_bt_gard->isEnabled())
			return;

		c_bt_gard->setEnabled(true);

		TCompHealth* c_health_gard = e_gard->get<TCompHealth>();
		assert(c_health_gard);
		c_health_gard->setHealth(c_health_gard->getMaxHealth());
		c_health_gard->setRenderActive(true);

		EngineLua.executeScript(_name + "Area()");

		// why do i hear boss music
		TCompGameManager* h_game_manager = GameManager->get<TCompGameManager>();
		auto& boss_state = h_game_manager->getBossStateByName("Gard");
		if (boss_state.music_event == nullptr) {
			EngineAudio.postMusicEvent("Music/Boss_Theme");
			boss_state.music_event = EngineAudio.getCurMusicEvent();
		}
		EngineAudio.setGlobalRTPC("Gard_Phase", 1, true);

		{
			CEntity* player = getEntityByName("player");
			TCompCollider* c_collider = player->get<TCompCollider>();
			c_collider->enableBoxController();
		}

		// Cinematics??
		// ...

		VHandles lianas = CTagsManager::get().getAllEntitiesByTag("blocking_lianas");
		for (auto h : lianas) {
			assert(h.isValid());
			CEntity* e = h;
			TCompSkeleton* comp_skel = e->get<TCompSkeleton>();
			comp_skel->resume(0.8f);
		}
	}
};

class CTriggerAreaCygnusArena : public ITriggerArea {

public:
	CTriggerAreaCygnusArena(const std::string& name) : ITriggerArea(name) {}

	void onAreaEnter(CHandle event_trigger, CHandle observer) override
	{
		{
			CEntity* e_cygnus = getEntityByName("Cygnus_Form_1");
			if (!e_cygnus)
				return;
			TCompBT* c_bt = e_cygnus->get<TCompBT>();
			assert(c_bt);

			if (c_bt->isEnabled())
				return;

			c_bt->setEnabled(true);

			TCompHealth* c_health = e_cygnus->get<TCompHealth>();
			assert(c_health);
			c_health->setHealth(c_health->getMaxHealth());
			c_health->setRenderActive(true);

			EngineLua.executeScript(_name + "Area()");

			// TODO: Play Cygnus music
			TCompGameManager* h_game_manager = GameManager->get<TCompGameManager>();
			auto& boss_state = h_game_manager->getBossStateByName("Cygnus");
			if (boss_state.music_event == nullptr) {
				EngineAudio.postMusicEvent("Music/Boss_Theme");
				boss_state.music_event = EngineAudio.getCurMusicEvent();
			}
			EngineAudio.setGlobalRTPC("Gard_Phase", 1, true);

			// Audio: disable Eon as a music interactor, as Cygnus will be now the one who manages it
			CEntity* e_eon = getEntityByName("player");
			TCompMusicInteractor* h_mus_int = e_eon->get<TCompMusicInteractor>();
			h_mus_int->setEnabled(false);
		}
	}
};
#pragma endregion


#pragma region UI Messages

class CTriggerAreaDisplayMessage : public ITriggerArea {

public:
	CTriggerAreaDisplayMessage(const std::string& name) : ITriggerArea(name) {}

	void onAreaEnter(CHandle event_trigger, CHandle observer) override
	{
		CEntity* e_trigger_area = event_trigger;
		e_trigger_area->sendMsg(TMsgActivateMsgArea());
	}

	void onAreaExit(CHandle event_trigger, CHandle observer) override
	{
		CEntity* e_trigger_area = event_trigger;
		e_trigger_area->sendMsg(TMsgDeactivateMsgArea());
	}
};

#pragma endregion

#pragma region Cinematics
class CTriggerAreaTestCinematic : public ITriggerArea {

public:
	CTriggerAreaTestCinematic(const std::string& name) : ITriggerArea(name) {}

	void onAreaEnter(CHandle event_trigger, CHandle observer) override
	{
		EngineLua.executeScript(_name + "Area()");
	}
};
#pragma endregion

#pragma region Cameras
class CTriggerAreaHighCamera : public ITriggerArea {
public:
	CTriggerAreaHighCamera(const std::string& name) : ITriggerArea(name) {}

	// Trigger will always be Eon: is restricted in the CompTriggerArea
	void onAreaEnter(CHandle event_trigger, CHandle observer) override
	{
		CEntity* player = getAreaTrigger(event_trigger);
		TCompPlayerController* controller = player->get<TCompPlayerController>();
		controller->blockAim();
		controller->blendCamera("camera_follow_high", 1.3f, &interpolators::quadInOutInterpolator);
	}

	void onAreaExit(CHandle event_trigger, CHandle observer) override
	{
		CEntity* player = getAreaTrigger(event_trigger);
		TCompPlayerController* controller = player->get<TCompPlayerController>();
		controller->unBlockAim();
		controller->blendCamera("camera_follow", 1.4f, &interpolators::quadInOutInterpolator);
	}
};

class CTriggerAreaDeathCameraArea : public ITriggerArea {

public:
	CTriggerAreaDeathCameraArea(const std::string& name) : ITriggerArea(name) {}

	void onAreaEnter(CHandle event_trigger, CHandle observer) override
	{
		CEntity* e_camera = getEntityByName("camera_follow");
		assert(e_camera);
		TCompCameraFollow* c_follow = e_camera->get<TCompCameraFollow>();
		assert(c_follow);
		c_follow->target_dead = true;
	}
};

#pragma endregion


#define REGISTER_TRIGGER_AREA(AREA_NAME)  {new CTriggerArea##AREA_NAME(#AREA_NAME)},
std::vector<ITriggerArea*> TCompGameManager::trigger_areas {
	#include "components/gameplay/trigger_area_registration.h"
};
#undef REGISTER_TRIGGER_AREA
