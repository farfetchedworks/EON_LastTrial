#pragma once

#include "entity/entity.h"
#include "components/common/comp_base.h"
#include "components/cameras/comp_gameplay_camera.h"
#include "components/messages.h"
#include "fmod_studio.hpp"

class ITriggerArea;
struct TCompGameplayCamera;

struct SBossState {
	std::string energy_wall;
	std::string trigger_area;
	bool is_dead = false;

	// Music event to release and stop in the game manager
	FMOD::Studio::EventInstance* music_event = nullptr;
};

enum class eLOCATION{
	CAVE = 0,
	TEMPLE,
	RIFT,
	GARDEN
};

class TCompGameManager : public TCompBase {

	DECL_SIBLING_ACCESS();

public:

	enum ETimeStatus {
		NORMAL,
		SLOW,
		PAUSE,
		SIZE
	};

	enum EParameterDDA {
		MAX_HP,
		HEAL_AMOUNT,
		COOLDOWN,
		DAMAGE,
		SIZE_PARAMS
	};

private:

	// default function for not restricting parseWithTag method
	std::function<bool(const std::string&)> parse_all = [](const std::string&) {
		return true;
	};

	std::map<std::string, CHandle> _cameras;        // Contains map of different Game play cameras (TCompFollow, etc)
	CHandle _lastUsedCamera;                        // Entity storing last camera used

	std::vector<std::string> boot;
	VHandles _shrines;
	bool eonHasDied = false;
	float deathTimer = 0.f;              // On Eon died use a timer if the player wants to resurrect
	float timeTillDeath = 5.f;           // After this time Eon is considered dead and the level will restart

	bool show_end_game = false;
	float show_end_game_timer = 10.0f;

	ETimeStatus time_status = ETimeStatus::NORMAL;
	float time_status_timings[ETimeStatus::SIZE] = {};

	// Used to interpolate time changes
	ETimeStatus target_time_status = ETimeStatus::NORMAL;
	float lerp_seconds = 1.0f;
	float lerp_accum = 0.0f;
	bool interpolating_time = false;

	void manageEonDeath(float dt);                  
	void registerGameEvent(const std::string& name, std::function<void(CHandle, CHandle)> callback);
	void registerTriggerAreaEvents();
	void notifyEonDeath(bool isEonDead = true);
	
	// Stores the messages that will be displayed in the UI
	void parseMessages(const std::string& filename);

	static std::map<std::string, unsigned int> gameEvents;
	static std::vector<ITriggerArea*> trigger_areas;

	std::map<std::string, SBossState> bosses_states;								// manage bosses states

	bool is_in_cinematic = false;
	eLOCATION player_location = eLOCATION::CAVE;

	// For Dynamic Difficulty Adjustment
	int max_deaths_easy = 10;
	float dda_parameters[EParameterDDA::SIZE_PARAMS] = {0.75, 0.75, 6.0, 0.75};
	int deaths_num = 0;

public:

	// List of messages to be displayed in the UI
	std::map<std::string, std::string> ui_messages;

	void onEonHasDied(const TMsgEonHasDied& msg);
	void onEonHasRevived(const TMsgEonHasRevived& msg);
	void onShrineCreated(const TMsgShrineCreated& msg);
	void onCameraCreated(const TMsgCameraCreated& msg);
	void onUnregisterGameEvent(const TMsgUnregEvent& msg);
	void onBossDead(const TMsgBossDead& msg);

	void setPlayerLocation(eLOCATION location) { player_location = location; }
	void setIsInCinematic(bool value);
	bool isInCinematic();

	// Register messages to be called by other entities
	static void registerMsgs() {
		DECL_MSG(TCompGameManager, TMsgAllEntitiesCreated, onAllEntitiesCreated);
		DECL_MSG(TCompGameManager, TMsgEonHasDied, onEonHasDied);
		DECL_MSG(TCompGameManager, TMsgEonHasRevived, onEonHasRevived);
		DECL_MSG(TCompGameManager, TMsgShrineCreated, onShrineCreated);
		DECL_MSG(TCompGameManager, TMsgCameraCreated, onCameraCreated);
		DECL_MSG(TCompGameManager, TMsgUnregEvent, onUnregisterGameEvent);
		DECL_MSG(TCompGameManager, TMsgBossDead, onBossDead);
	}

	void load(const json& j, TEntityParseContext& ctx);
	void onAllEntitiesCreated(const TMsgAllEntitiesCreated& msg);
	void update(float dt);
	void debugInMenu();

	void enableCamera(const std::string& activeCamera);
	void clearCameraDeltas();
	void respawnLevel();
	void respawnEnemies();
	void restartLevel();

	bool isGardKilled();

	bool		isEonDied() { return eonHasDied; };
	float		getTimeScaleFactor() { return 1.0f; };
	ETimeStatus	getTimeStatus() { return time_status; };
	CHandle		getLastUsedCamera() { return _lastUsedCamera; };
	eLOCATION	getPlayerLocation() { return player_location; }

	bool		  isBossValid(const std::string& name);
	SBossState&   getBossStateByName(const std::string& name);

	void setTimeStatus(ETimeStatus status);
	void setTimeStatusLerped(ETimeStatus status, float seconds);
	void setEonDied();

	// DDA methods
	float getAdjustedParameter(EParameterDDA param_dda, float current_value);
};
