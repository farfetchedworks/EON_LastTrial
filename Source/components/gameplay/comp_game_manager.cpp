#include "mcv_platform.h"
#include "engine.h"
#include "components/messages.h"
#include "comp_game_manager.h"
#include "comp_shrine.h"
#include "comp_trigger_area.h"
#include "comp_energy_wall.h"
#include "entity/entity.h"
#include "entity/entity_parser.h"
#include "render/render_module.h"
#include "ui/ui_module.h"
#include "ui/ui_widget.h"
#include "modules/module_events.h"
#include "components/controllers/comp_pawn_controller.h"
#include "components/cameras/comp_camera_follow.h"
#include "components/common/comp_transform.h"
#include "components/common/comp_collider.h"
#include "components/common/comp_tags.h"
#include "components/stats/comp_geons_manager.h"
#include "components/stats/comp_health.h"
#include "components/render/comp_dissolve.h"
#include "components/postfx/comp_motion_blur.h"
#include "audio/module_audio.h"
#include "geometry/interpolators.h"
#include "components/ai/comp_bt.h"

DECL_OBJ_MANAGER("game_manager", TCompGameManager)

std::map<std::string, unsigned int> TCompGameManager::gameEvents;

void TCompGameManager::load(const json& j, TEntityParseContext& ctx)
{
	assert(j["boot"].is_array());
	boot = j["boot"].get< std::vector< std::string > >();

	timeTillDeath = j.value("death_timer", timeTillDeath);
	time_status_timings[ETimeStatus::NORMAL] = 1.0f;
	time_status_timings[ETimeStatus::SLOW] = j.value("time_scale_factor", 0.2f);
	time_status_timings[ETimeStatus::SLOWEST] = 0.1f;
	time_status_timings[ETimeStatus::PAUSE] = 0.0f;

	for (const json& jBoss : j["bosses"]) {
		SBossState boss_state;
		boss_state.energy_wall = jBoss["energy_wall"];
		boss_state.trigger_area = jBoss["trigger_area"];
		bosses_states[jBoss["name"]] = boss_state;
	}

	parseMessages("data/ui/messages_list.json");

	// DDA parameters
	max_deaths_easy = j.value("max_deaths_easy", max_deaths_easy);
	dda_parameters[EParameterDDA::MAX_HP] = j.value("max_HP_redux_perc", dda_parameters[EParameterDDA::MAX_HP]);
	dda_parameters[EParameterDDA::HEAL_AMOUNT] = j.value("max_heal_redux_perc", dda_parameters[EParameterDDA::HEAL_AMOUNT]);
	dda_parameters[EParameterDDA::COOLDOWN] = j.value("max_cooldown_inc", dda_parameters[EParameterDDA::COOLDOWN]);
	dda_parameters[EParameterDDA::DAMAGE] = j.value("max_damage_redux_perc", dda_parameters[EParameterDDA::DAMAGE]);
}

void TCompGameManager::onAllEntitiesCreated(const TMsgAllEntitiesCreated& msg)
{
	registerTriggerAreaEvents();
}

void TCompGameManager::toLoading()
{
	CModuleManager& modules = CEngine::get().getModuleManager();
	modules.changeToGamestate("loading");
}

void TCompGameManager::update(float dt)
{
	if (interpolating_time)
	{
		float time_lerp_source = time_status_timings[time_status];
		float time_lerp_target = time_status_timings[target_time_status];
		lerp_accum += Time.delta_unscaled / lerp_seconds;

		float f = clampf(lerp_accum, 0.f, 1.f);

		if (time_interpolator)
			Time.scale_factor = lerp(time_lerp_source, time_lerp_target, time_interpolator->blend(0.f, 1.f, f));
		else
			Time.scale_factor = lerp(time_lerp_source, time_lerp_target, f);

		if (lerp_accum >= 1.f)
		{
			interpolating_time = false;
			time_interpolator = nullptr;
			lerp_accum = 0.0f;
			setTimeStatus(target_time_status);
		}
	}

	if (eonHasDied) {
		manageEonDeath(dt);
	}
}

void TCompGameManager::setTimeStatus(ETimeStatus status)
{
	time_status = status;
	Time.scale_factor = time_status_timings[time_status];
}

void TCompGameManager::setTimeStatusLerped(ETimeStatus status, float seconds, const interpolators::IInterpolator* it)
{
	if (seconds == 0.f) {
		setTimeStatus(status);
	}
	else {
		lerp_seconds = seconds;
		interpolating_time = true;
		target_time_status = status;

		if (it)
			setTimeInterpolator(it);
	}
}

void TCompGameManager::setTimeInterpolator(const interpolators::IInterpolator* it)
{
	time_interpolator = it;
}

void TCompGameManager::setEonDied()
{
	setTimeStatus(ETimeStatus::SLOW);
	deathTimer = 0.0f;
	eonHasDied = true;

	// This is when animation ends
	CEntity* player = getEntityByName("player");
	assert(player);
	TCompDissolveEffect* c_dissolve = player->get<TCompDissolveEffect>();
	if (c_dissolve) {
		c_dissolve->enable(7.f);
	}
}

// Restarts the current level
void TCompGameManager::restartLevel()
{
	eonHasDied = false;
	eonDeathManaged = false;

	CEntity* e_camera_follow = getEntityByName("camera_follow");
	EngineRender.setActiveCamera(e_camera_follow);
	TCompCameraFollow* c_camera_follow = e_camera_follow->get<TCompCameraFollow>();
	c_camera_follow->reset(true);

	// Get eon's phase before respawning
	CEntity* e_player = getEntityByName("player");
	TCompGeonsManager* comp_geons = e_player->get<TCompGeonsManager>();
	int lastEonPhase = comp_geons->getPhase();

	// Destroy and respawn level
	respawnLevel();

	// entity pointer is different now
	e_player = getEntityByName("player");

	// Set last phase -> no geons
	comp_geons = e_player->get<TCompGeonsManager>();
	comp_geons->setPhase(lastEonPhase);

	auto it = rbegin(_shrines);
	for (; it != rend(_shrines); ++it)
	{
		TCompShrine* shrine = *it;
		assert(shrine);

		if (!shrine->isActive())
			continue;

		TCompCollider* collider_player = e_player->get<TCompCollider>();
		e_player->setPosition(shrine->getPrayPos().player);
		collider_player->setControllerPosition(shrine->getPrayPos().collider);
		shrine->restorePlayer();

		// No more checks
		break;
	}

	// Spawn Eon in the player start position
	// if no active shrine is found
	if (it == rend(_shrines)) {
		VHandles v_player_start = CTagsManager::get().getAllEntitiesByTag("player_start");

		if (v_player_start.empty()) {
			e_player->setPosition(VEC3::Zero, true);
		}
		else {
			CEntity* e_player_start = v_player_start[0];
			TCompTransform* h_start_trans = e_player_start->get<TCompTransform>();
			TCompTransform* h_player_trans = e_player->get<TCompTransform>();

			// only assign pos and rotation (not scale!!!)
			e_player->setPosition(h_start_trans->getPosition(), true);
			h_player_trans->setRotation(h_start_trans->getRotation());
		}
	}
}

void TCompGameManager::respawnLevel()
{
	VHandles entities_to_destroy = CTagsManager::get().getAllEntitiesByTag("not_persistent");

	for (auto h : entities_to_destroy)
	{
		if (h.isValid())
			h.destroy();
	}

	CHandleManager::destroyAllPendingObjects();

	{
		TEntityParseContext ctx;
		parseScene("data/scenes/eon.json", ctx);
		TMsgAllEntitiesCreated msg;
		for (auto& h : ctx.entities_loaded)
			h.sendMsg(msg);
	}

	// Spawn level + Bosses
	{
		for (auto& b : boot) {
			
			TEntityParseContext ctx;
			parseSceneWithTag(b, "not_persistent", ctx, [&](const std::string& name) {
				if (isBossValid(name))
					return !bosses_states[name].is_dead;
				return true;
			});

			TMsgAllEntitiesCreated msg;
			for (auto& h : ctx.entities_loaded)
				h.sendMsg(msg);
		}
	}

	// Reset boss doors in case of respawning when the boss is still alive
	{
		for (auto& boss_state : bosses_states) {

			if (boss_state.second.is_dead)
				continue;

			CEntity* e_energy_wall = getEntityByName(boss_state.second.energy_wall);
			if (e_energy_wall) {
				CHandle h_energyWall = e_energy_wall->get<TCompEnergyWall>();
				static_cast<TCompEnergyWall*>(h_energyWall)->reset();
			}

			// End boss music
			if (boss_state.second.music_event) {
				boss_state.second.music_event->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
				boss_state.second.music_event->release();
				boss_state.second.music_event = nullptr;
			}
		}
	}
}

void TCompGameManager::respawnEnemies()
{
	// Respawn enemies
	VHandles entities_to_destroy = CTagsManager::get().getAllEntitiesByTag("enemy");

	for (auto h : entities_to_destroy)
	{
		if (h.isValid())
			h.destroy();
	}

	CHandleManager::destroyAllPendingObjects();

	for (auto& b : boot) {

		TEntityParseContext ctx;
		parseSceneWithTag(b, "enemy", ctx, parse_all);
	}
}

void TCompGameManager::manageEonDeath(float dt)
{
	deathTimer += (dt / time_status_timings[ETimeStatus::SLOW]);
	if (deathTimer >= timeTillDeath && !eonDeathManaged) {

		// If Eon hasn't revived, increase the amount of deaths
		deaths_num++;
		eonDeathManaged = true;
		
		// Don't restart automatically
		// restartLevel();

		setTimeStatus(ETimeStatus::NORMAL);
		EngineUI.deactivateWidget("boss_life_bar");

		CModuleManager& modules = CEngine::get().getModuleManager();
		modules.changeToGamestate("you_died");
	}
}

void TCompGameManager::enableCamera(const std::string& activeCamera)
{
	for (auto& cam : _cameras) {

		CEntity* owner = cam.second.getOwner();
		std::string cameraName = owner->getName();

		IGameplayCamera* gameCamera = IGameplayCamera::fromCamera(owner);
		assert(gameCamera);
		bool isActive = (cameraName == activeCamera);
		gameCamera->enabled = isActive;

		if (isActive) {
			_lastUsedCamera = IGameplayCamera::getHandle(gameCamera);
		}
	}
}

void TCompGameManager::clearCameraDeltas()
{
	for (auto& cam : _cameras) {

		CEntity* owner = cam.second.getOwner();
		std::string cameraName = owner->getName();

		IGameplayCamera* gameCamera = IGameplayCamera::fromCamera(owner);
		assert(gameCamera);
		gameCamera->clearDeltas();
	}
}

void TCompGameManager::registerTriggerAreaEvents()
{
	for (auto& area : trigger_areas)
	{
		const std::string& name = area->getName();

		registerGameEvent("TriggerEnter@" + name,
			std::bind(&ITriggerArea::onAreaEnter, area, std::placeholders::_1, std::placeholders::_2));

		registerGameEvent("TriggerExit@" + name,
			std::bind(&ITriggerArea::onAreaExit, area, std::placeholders::_1, std::placeholders::_2));
	}
}

void TCompGameManager::registerGameEvent(const std::string& name, std::function<void(CHandle, CHandle)> callback)
{
	auto id = EventSystem.registerEventCallback(name, callback);
	gameEvents[name] = id;
}

void TCompGameManager::parseMessages(const std::string& filename) {
	const json jData = loadJson(filename);
	for (const json& jFileEntry : jData) {
		for (const json& jMessage : jFileEntry["messages"]) {
			ui_messages[jFileEntry["id"]] = jMessage["data"];
		}
	}
}

bool TCompGameManager::isBossValid(const std::string& name)
{
	auto boss_state = bosses_states.find(name);
	if (boss_state != bosses_states.end())
		return true;

	return false;
}

SBossState& TCompGameManager::getBossStateByName(const std::string& name)
{
	assert(isBossValid(name));
	return bosses_states[name];
}

/*
*	Messages
*/

void TCompGameManager::onUnregisterGameEvent(const TMsgUnregEvent& msg)
{
	unsigned int id = gameEvents[msg.name];
	EventSystem.unregisterEventCallback(msg.name, id);
}

void TCompGameManager::onCameraCreated(const TMsgCameraCreated& msg)
{
	auto it = _cameras.find(msg.name);
	if (it != _cameras.end()) {
		dbg("Camera '%s' already added\n", msg.name.c_str());
		return;
	}

	_cameras[msg.name] = msg.camera;

	// set as default active
	if (msg.active)
		_lastUsedCamera = getEntityByName(msg.name);
}

void TCompGameManager::onEonHasDied(const TMsgEonHasDied& msg)
{
	CEntity* player = getEntityByName("player");
	if (!player)
		return;

	notifyEonDeath(true);

	TCompPawnController* pawn = player->get<TCompPawnController>();
	pawn->resetSpeedMultiplier();

	// Remove motion blur if needed (slow debuff)
	CEntity* camera = getEntityByName("camera_mixed");
	assert(camera);
	TCompMotionBlur* c_motion_blur = camera->get<TCompMotionBlur>();
	if (c_motion_blur && c_motion_blur->isEnabled()) {
		c_motion_blur->disable();
	}

	// Allow revival
	TMsgEonRevive msgEonRevive;
	player->sendMsg(msgEonRevive);

	// FMOD event
	EngineAudio.setGlobalRTPC("Eon_Dead", 1.f);
}

void TCompGameManager::onEonHasRevived(const TMsgEonHasRevived& msg)
{
	eonHasDied = false;

	notifyEonDeath(false);

	// Enable follow cam
	CEntity* e_camera = getEntityByName("camera_follow");
	assert(e_camera);
	TCompCameraFollow* c_follow = e_camera->get<TCompCameraFollow>();
	assert(c_follow);
	c_follow->target_dead = false;

	CEntity* player = getEntityByName("player");
	TCompDissolveEffect* c_dissolve = player->get<TCompDissolveEffect>();
	if (c_dissolve) {
		c_dissolve->recover();
	}

	// FMOD event
	EngineAudio.setGlobalRTPC("Eon_Dead", 0.f);
}

void TCompGameManager::onShrineCreated(const TMsgShrineCreated& msg)
{
	if (msg.h_shrine.isValid())
		_shrines.push_back(msg.h_shrine);
}

void TCompGameManager::onBossDead(const TMsgBossDead& msg)
{
	bool is_ok = isBossValid(msg.boss_name);
	assert(is_ok);
	if (!is_ok)
		return;
	
	auto& boss = bosses_states[msg.boss_name];
	boss.is_dead = true;

	if (msg.boss_name == "Gard") {

		//show_end_game = true;
		//show_end_game_timer = 10.0f;

		// End boss music
		EngineAudio.setGlobalRTPC("Gard_Phase", 4);
		boss.music_event->release();
	}
	else if (msg.boss_name == "Cygnus") {

		//show_end_game = true;
		//show_end_game_timer = 10.0f;

		// End boss music
		EngineAudio.setMusicRTPC("Cygnus_Phase", 4);
	}

	//CEntity* triggerArea = getEntityByName(boss.trigger_area);
	//TCompTriggerArea* area = triggerArea->get<TCompTriggerArea>();
	// ...

	CHandle hWall = getEntityByName(boss.energy_wall);
	if(hWall.isValid())
		hWall.destroy();
	CHandle hTrigger = getEntityByName(boss.trigger_area);
	if (hTrigger.isValid())
		hTrigger.destroy();
}

void TCompGameManager::setIsInCinematic(bool value)
{
	is_in_cinematic = value;
}

bool TCompGameManager::isInCinematic()
{
	return is_in_cinematic;
}

float TCompGameManager::getAdjustedParameter(EParameterDDA param_dda, float current_value) 
{
	float deaths_ratio = std::min(deaths_num, max_deaths_easy) / (float)max_deaths_easy;		// Calculate the ratio, considering that if it exceeds the max amount of deaths it will always be 1
	
	if (param_dda == EParameterDDA::COOLDOWN) 
		return 	interpolators::linear(current_value, current_value + dda_parameters[EParameterDDA::COOLDOWN], deaths_ratio);
	
	return interpolators::linear(current_value, current_value * (1 - dda_parameters[param_dda]), deaths_ratio);
}

void TCompGameManager::notifyEonDeath(bool isEonDead)
{
	getObjectManager<TCompBT>()->forEach(
		[&](TCompBT* h_comp_bt) {
			h_comp_bt->onEonIsDead(isEonDead);
		}
	);
}

void TCompGameManager::debugInMenu()
{
	if (ImGui::TreeNode("Bosses"))
	{
		for (const auto& boss : bosses_states)
		{
			ImGui::Text(boss.first.c_str());
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Cameras"))
	{
		for (const auto& c : _cameras)
		{
			CEntity* owner = c.second.getOwner();
			owner->debugInMenu();
		}

		if (_lastUsedCamera.isValid()) {
			ImGui::Separator();
			CEntity* active_cam = _lastUsedCamera;
			ImGui::Text(active_cam->getName());
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Shrines"))
	{
		for (const auto& s : _shrines)
		{
			TCompShrine* shrine = s;
			shrine->debugInMenu();
		}
		ImGui::TreePop();
	}
}