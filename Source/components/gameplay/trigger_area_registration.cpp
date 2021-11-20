#include "mcv_platform.h"
#include "handle/handle.h"
#include "engine.h"
#include "entity/entity.h"
#include "entity/entity_parser.h"
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
#include "components/common/comp_fsm.h"
#include "components/audio/comp_music_interactor.h"
#include "skeleton/comp_skeleton.h"
#include "lua/module_scripting.h"
#include "audio/module_audio.h"

#define PLAY_CINEMATICS true

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

		TCompFSM* fsm = e_gard->get<TCompFSM>();

		if (fsm->getCtx().isEnabled())
			return;

		fsm->startCtx();

		EngineLua.executeScript(_name + "Area()");

		// why do i hear boss music
		TCompGameManager* h_game_manager = GameManager->get<TCompGameManager>();
		auto& boss_state = h_game_manager->getBossStateByName("Gard");
		if (boss_state.music_event == nullptr) {
			EngineAudio.postMusicEvent("Music/Boss_Theme");
			boss_state.music_event = EngineAudio.getCurMusicEvent();
		}
		EngineAudio.setGlobalRTPC("Gard_Phase", 1, true);

		// Cinematics
		EngineLua.executeScript("CinematicGardPresentation()");
	}
};

class CTriggerAreaCygnusArena : public ITriggerArea {

public:
	CTriggerAreaCygnusArena(const std::string& name) : ITriggerArea(name) {}

	void onAreaEnter(CHandle event_trigger, CHandle observer) override
	{
		// Play Cygnus music
		TCompGameManager* h_game_manager = GameManager->get<TCompGameManager>();
		auto& boss_state = h_game_manager->getBossStateByName("Cygnus");
		if (boss_state.music_event != nullptr)
			return;

		EngineAudio.postMusicEvent("Music/Cygnus_Theme");
		boss_state.music_event = EngineAudio.getCurMusicEvent();
		EngineAudio.setMusicRTPC("Cygnus_Phase", 1, true);

		// Audio: disable Eon as a music interactor, as Cygnus will be now the one who manages it
		CEntity* e_eon = getEntityByName("player");
		TCompMusicInteractor* h_mus_int = e_eon->get<TCompMusicInteractor>();
		h_mus_int->setEnabled(false);

#if PLAY_CINEMATICS
		CEntity* e_cygnus = getEntityByName("Cygnus_Form_1");
		if (e_cygnus)
			return;

		CTransform t;

		// Cygnus shrine position
		{
			VHandles cShrineTags = CTagsManager::get().getAllEntitiesByTag(getID("SmegShrine"));
			assert(cShrineTags.size() == 1);
			t.setPosition(static_cast<CEntity*>(cShrineTags[0])->getPosition());
		}

		e_cygnus = spawn("data/prefabs/Cygnus_Form_1.json", t);
		assert(e_cygnus);

		TCompFSM* fsm = e_cygnus->get<TCompFSM>();
		fsm->startCtx();

		// Intro
		EngineLua.executeScript("CinematicCygnusPresentation()");

#else
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
#endif
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

#pragma region FMOD Volumes

class CTriggerAreaCaveArea : public ITriggerArea {
public:
	CTriggerAreaCaveArea(const std::string& name) : ITriggerArea(name) {}

	void onAreaEnter(CHandle event_trigger, CHandle observer) override
	{
		// Start ambience
		EngineAudio.postAmbienceEvent("AMB/Cave/cave_ambience");

		// Switch on cave reverb
		EngineAudio.setGlobalRTPC("Cave_Reverb", 1.0);

		// Set new location
		TCompGameManager* gm = GameManager->get<TCompGameManager>();
		gm->setPlayerLocation(eLOCATION::CAVE);
	}

	void onAreaExit(CHandle event_trigger, CHandle observer) override
	{
		// Switch off cave reverb
		EngineAudio.setGlobalRTPC("Cave_Reverb", 0.0);
	}
};

class CTriggerAreaRiftArea : public ITriggerArea {
public:
	CTriggerAreaRiftArea(const std::string& name) : ITriggerArea(name) {}

	void onAreaEnter(CHandle event_trigger, CHandle observer) override
	{
		// Activate music interaction
		CEntity* e_owner = getEntityByName("player");
		TCompMusicInteractor* t_mus_int = e_owner->get<TCompMusicInteractor>();
		t_mus_int->setEnabled(true);

		// Start temple music
		EngineAudio.postMusicEvent("Music/Enter_Rift_Theme");
		EngineLua.executeScript("fmodPostMusicEvent(\"Music/Temple_Theme\")", 9.5f);

		// Set new location
		TCompGameManager* gm = GameManager->get<TCompGameManager>();
		gm->setPlayerLocation(eLOCATION::RIFT);

		// Inform the game manager that Eon has entered the rift at least once, music purposes
		gm->setHasEnteredRiftOnce(true);
	}

	void onAreaExit(CHandle event_trigger, CHandle observer) override
	{
		// dbg("EXIT RIFT");
	}
};

class CTriggerAreaTempleArea : public ITriggerArea {
public:
	CTriggerAreaTempleArea(const std::string& name) : ITriggerArea(name) {}

	void onAreaEnter(CHandle event_trigger, CHandle observer) override
	{
		// Start ambience
		EngineAudio.postAmbienceEvent("AMB/Temple/temple_ambience");

		// Switch on temple reverb
		EngineAudio.setGlobalRTPC("Monastery_Reverb", 1.0);

		// Set new location
		TCompGameManager* gm = GameManager->get<TCompGameManager>();
		gm->setPlayerLocation(eLOCATION::TEMPLE);

		// If Eon already entered the rift for the 1st time (therefore he died at some point), play music
		if (gm->getHasHasEnteredRiftOnce()) {
			// Activate music interaction
			CEntity* e_owner = getEntityByName("player");
			TCompMusicInteractor* t_mus_int = e_owner->get<TCompMusicInteractor>();
			t_mus_int->setEnabled(true);

			EngineAudio.postMusicEvent("Music/Temple_Theme");
		}
	}

	void onAreaExit(CHandle event_trigger, CHandle observer) override
	{
		EngineAudio.setGlobalRTPC("Monastery_Reverb", 0.0);
	}
};

class CTriggerAreaGardenArea : public ITriggerArea {
public:
	CTriggerAreaGardenArea(const std::string& name) : ITriggerArea(name) {}

	void onAreaEnter(CHandle event_trigger, CHandle observer) override
	{
		// Start ambience
		EngineAudio.postAmbienceEvent("AMB/Temple/garden_ambience");

		// Set new location
		TCompGameManager* gm = GameManager->get<TCompGameManager>();
		gm->setPlayerLocation(eLOCATION::GARDEN);
	}

	void onAreaExit(CHandle event_trigger, CHandle observer) override
	{
		EngineAudio.postAmbienceEvent("AMB/Temple/temple_ambience");
	}
};

#pragma endregion

#define REGISTER_TRIGGER_AREA(AREA_NAME)  {new CTriggerArea##AREA_NAME(#AREA_NAME)},
std::vector<ITriggerArea*> TCompGameManager::trigger_areas {
	#include "components/gameplay/trigger_area_registration.h"
};
#undef REGISTER_TRIGGER_AREA
