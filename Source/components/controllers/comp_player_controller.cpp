#include "mcv_platform.h"
#include "comp_player_controller.h"
#include "engine.h"
#include "entity/entity_parser.h"
#include "render/render_module.h"
#include "render/draw_primitives.h"
#include "input/input_module.h"
#include "lua/module_scripting.h"
#include "modules/game/module_player_interaction.h"
#include "audio/module_audio.h"
#include "audio/physical_material.h"
#include "modules/module_physics.h"
#include "modules/module_camera_mixer.h"
#include "modules/module_entities.h"
#include "navmesh/module_navmesh.h"
#include "fsm/states/logic/state_logic.h"
#include "components/common/comp_transform.h"
#include "components/common/comp_render.h"
#include "components/common/comp_camera.h"
#include "components/common/comp_collider.h"
#include "components/common/comp_fsm.h"
#include "components/abilities/comp_time_reversal.h"
#include "components/abilities/comp_area_delay.h"
#include "components/abilities/comp_heal.h"
#include "components/stats/comp_stamina.h"
#include "components/stats/comp_health.h"
#include "components/stats/comp_warp_energy.h"
#include "components/stats/comp_attributes.h"
#include "components/stats/comp_geons_manager.h"
#include "components/cameras/comp_camera_follow.h"
#include "components/cameras/comp_camera_shooter.h"
#include "components/gameplay/comp_game_manager.h"
#include "components/ai/comp_bt.h"
#include "bt/task_utils.h"
#include "components/controllers/pawn_utils.h"
#include "components/common/comp_hierarchy.h"
#include "components/common/comp_collider.h"
#include "components/combat/comp_lock_target.h"
#include "ui/ui_module.h"
#include "ui/ui_widget.h"

#include "../bin/data/shaders/constants_particles.h"

// Uncomment this and use the first option of the
// "sprint" mapping to use same button for both 
// #define SAME_SPRINT_DASH_BTN

DECL_OBJ_MANAGER("player_controller", TCompPlayerController)

extern CShaderCte<CtesWorld> cte_world;

void TCompPlayerController::load(const json& j, TEntityParseContext& ctx)
{
	TCompPawnController::load(j, ctx);

	// Movement
	move_speed = j.value("move_speed", move_speed);
	current_speed = move_speed;
	walk_speed = j.value("walk_speed", walk_speed);
	sprint_speed = j.value("sprint_speed", sprint_speed);
	rotation_speed = j.value("rotation_speed", rotation_speed);
	speed_mod = j.value("speed_mod", speed_mod);
	block_attacks = j.value("block_attacks", block_attacks);

	dash_force = j.value("dash_force", dash_force);

	// Attack
	attack_range = j.value("attack_range", attack_range);

	// Lock-On Weights
	w_height_diff = j.value("w_height_diff", w_height_diff);
	w_alignment = j.value("w_alignment", w_alignment);
	w_distance = j.value("w_distance", w_distance);
}

void TCompPlayerController::onEntityCreated()
{
	TCompPawnController::onOwnerEntityCreated(getEntity());

	h_transform = get<TCompTransform>();
	h_time_reversal = get<TCompTimeReversal>();
	h_player_stamina = get<TCompStamina>();
	h_player_warp_energy = get<TCompWarpEnergy>();
	h_collider = get<TCompCollider>();
}

void TCompPlayerController::update(float dt)
{
	if (!enabled)
		return;

	TCompTransform* transform = h_transform;
	cte_world.player_pos = transform->getPosition();
	cte_world.player_dead = std::get<bool>(getVariable("is_death")) ? (cte_world.player_dead + dt * 0.7f) : 0.f;

	dt *= speed_multiplier;

	bool isFlyoverEnabled = isCameraEnabled("camera_flyover");

	// Flyover camera (F6)
	if (PlayerInput[VK_F6].getsPressed()) {
		toggleFlyover(isFlyoverEnabled);
	}

	if (isFlyoverEnabled) return;

	VEC2 blend_speed = getBlendSpeed();

	if (_lastSpeed != blend_speed) {
		_lastSpeed = blend_speed;

		setVariable("blend_factor", blend_speed);
	}

	if (currentPath.size())
	{
		updatePath(dt);
		return;
	}

	// Check if any change to avoid sending msgs every frame
	if (_lastIsMoving != is_moving) {
		_lastIsMoving = is_moving;
		setVariable("is_walking", is_moving);
	}

	// Call this always to read state from the component
	static_cast<TCompStamina*>(h_player_stamina)->recoverStamina(can_recover_stamina, dt);

	TCompTimeReversal* c_time_reversal = h_time_reversal;
	TCompWarpEnergy* c_warp_energy = h_player_warp_energy;
	
	bool is_praying	= std::get<bool>(getVariable("is_praying"));
	bool is_crossing_wall = std::get<bool>(getVariable("is_crossing_wall"));
	if (PlayerInput["time_reversal"].getsPressed() && c_warp_energy->hasWarpEnergy(c_time_reversal->getWarpConsumption()) && !is_crossing_wall && !is_praying) {
		TCompGameManager* gm = GameManager->get<TCompGameManager>();
		gm->setTimeStatusLerped(TCompGameManager::ETimeStatus::SLOW, reversal_timer_max);
	}

	if (PlayerInput["time_reversal"].isPressed() && allow_time_reversal
		&& c_warp_energy->hasWarpEnergy(c_time_reversal->getWarpConsumption())
		&& !is_praying
		&& !is_crossing_wall) {

		reversal_start_timer += std::min(Time.delta_unscaled, 0.05f);

		if (reversal_start_timer >= reversal_timer_max) {
			if (!is_aiming && !c_time_reversal->isRewinding() && c_time_reversal->startRewinding()) {
				allow_time_reversal = false;
				setVariable("in_time_reversal", true);
			}
		}
	}

	if (PlayerInput["time_reversal"].getsReleased())
	{
		// Time delta is scaled but time reversal was cancelled before starting
		if (allow_time_reversal) {
			TCompGameManager* gm = GameManager->get<TCompGameManager>();
			gm->setTimeStatusLerped(TCompGameManager::ETimeStatus::NORMAL, reversal_timer_max);
		}

		reversal_start_timer = 0.f;
		allow_time_reversal = true;
		last_dir = VEC3::Zero;
	}

	// mix shooter camera
	manageAimCamera();

	// Lock on
	if (PlayerInput["lock_on"].getsPressed()) {
		manageLockOn();
	}

	if (checkSprintInput()) {
		can_sprint = true;
	}

	if (PlayerInput["sprint"].getsReleased()) {
		can_sprint = false;
		can_continue_sprinting = false;
	}

	/*
	*   DEBUG SHORTCUTS
	*/

	// Disable AI (F2)
	if (PlayerInput[VK_F2].getsPressed()) {
		TCompBT::UPDATE_ENABLED = !TCompBT::UPDATE_ENABLED;
	}

	// Restore player (F3)
	if (PlayerInput[VK_F3].getsPressed()) {
		static_cast<TCompHealth*>(get<TCompHealth>())->fillHealth();
		static_cast<TCompStamina*>(get<TCompStamina>())->fillStamina();
		static_cast<TCompWarpEnergy*>(get<TCompWarpEnergy>())->fillWarpEnergy();
	}

	// Toggle invencible (F4)
	if (PlayerInput[VK_F4].getsPressed()) {
		GOD_MODE = !GOD_MODE;
	}

	// Superspeed toggle (F5)
	if (PlayerInput[VK_F5].getsPressed()) {
		static bool activateSuperspeed = true;
		static float orig_speed = current_speed;
		static float orig_move_speed = move_speed;
		static float orig_sprint_speed = sprint_speed;
		if (activateSuperspeed) {
			current_speed = 15.f;
			move_speed = 15.f;
			sprint_speed = 25.f;
		}
		else {
			current_speed = orig_speed;
			move_speed = orig_move_speed;
			sprint_speed = orig_sprint_speed;
		}
		activateSuperspeed = !activateSuperspeed;
	}

	if (PlayerInput[VK_F7].getsPressed()) {
		Entities.clearDebugList();
		Entities.enableBasicDebug();													// Display the minimum UI for debug
	}

	if (PlayerInput['C'].getsPressed()) {

		/*CTransform t;
		t.setPosition(getEntity()->getPosition());
		spawn("data/prefabs/Eter_Entero.json", t);*/

		TCompGeonsManager* geons_man = get<TCompGeonsManager >();
		geons_man->increasePhase();
	}

	// Auto-Kill (K)
	if (PlayerInput['K'].getsPressed()) {

		CEntity* player = getEntity();

		if (is_locked_on) {

			CEntity* enemy = h_locked_transform.getOwner();
			TMsgHit msgHit;
			msgHit.damage = 10000;
			msgHit.hitByPlayer = true;
			msgHit.h_striker = player;
			enemy->sendMsg(msgHit);
		}
		else
		{
			TMsgHit msgHit;
			msgHit.damage = 10000;
			msgHit.h_striker = player;
			player->sendMsg(msgHit);
		}
	}

	if (is_locked_on) {

		TCompTransform* c_locked_trans = h_locked_transform;
		// If the locked transform is no longer valid, remove the lock-on
		if (!h_locked_transform.isValid()) {
			removeLockOn();
			return;
		}

		VEC3 player_pos = transform->getPosition();
		VEC3 locked_pos = c_locked_trans->getPosition();

		float distance2d = (VEC2(locked_pos.x, locked_pos.z) - VEC2(player_pos.x, player_pos.z)).Length();

		if (distance2d < 5.f && std::fabs(player_pos.y - locked_pos.y) > 2.f)
			removeLockOn();
	}
}

bool TCompPlayerController::toggleFlyover(bool was_enabled)
{
	// Keep positions in sync
	{
		CEntity* camera = getEntityByName("camera_mixed");
		bool shiftPressed = isPressed(VK_SHIFT);
		if (shiftPressed && was_enabled) {
			PawnUtils::setPosition(getEntityByName("player"), camera->getPosition());
		}
		else if (shiftPressed && !was_enabled) {
			CEntity* flyover = getEntityByName("camera_flyover");
			assert(flyover);
			TCompTransform* last_cam_t = camera->get<TCompTransform>();
			TCompTransform* flyover_t = flyover->get<TCompTransform>();
			flyover_t->fromMatrix(*last_cam_t);
		}
	}

	// Blend cameras
	{
		if (!was_enabled) {
			const TMixedCamera& last_used_camera = CameraMixer.getLastCamera();
			last_camera = last_used_camera.entity;
			assert(last_camera.isValid());
			blendCamera("camera_flyover", 0.f);
		}
		else if(last_camera.isValid()){
			CEntity* e = last_camera;
			assert(e);
			blendCamera(e->getName(), 0.f);
		}
	}

	debugging = !was_enabled;
	CApplication::get().changeMouseState(debugging);
	PlayerInput.toggleBlockInput();
	return debugging;
}

VEC3 TCompPlayerController::getMoveDirection(bool& moving)
{
	VEC3 moveDir = VEC3::Zero;
	speed_multiplier_move = 0.0f;

	TCompGameManager* c_gm = GameManager->get<TCompGameManager>();

	if ((PlayerInteraction.isEnergyWallActive()) ||
		c_gm->isInCinematic() || isCameraEnabled("camera_shooter"))
			return moveDir;

	// Gamepad queries
	if (PlayerInput.getPad().connected && PlayerInput["move_joystick"].isPressed()) {
		VEC3 pad_move = VEC3(-PlayerInput[input::EPadButton::LANALOG_X].value, 0,
			PlayerInput[input::EPadButton::LANALOG_Y].value);
		speed_multiplier_move = pad_move.Length();
		moveDir += pad_move;
	}

	if (PlayerInput["move_keyboard"].timeSinceReleased() < 0.1f) {

		speed_multiplier_move = 1.0f;
		if (PlayerInput["move_keyboard"].isPressed())
			prev_target_speed = target_speed;
	}

	if (PlayerInput["move_forward"].isPressed()) {
		moveDir += VEC3(0, 0, 1);
	}
	else if (PlayerInput["move_back"].isPressed()) {
		moveDir -= VEC3(0, 0, 1);
	}

	if (PlayerInput["move_left"].isPressed()) {
		moveDir += VEC3(1, 0, 0);
	}
	else if (PlayerInput["move_right"].isPressed()) {
		moveDir -= VEC3(1, 0, 0);
	}

	raw_dir = moveDir;
	moving = moveDir.LengthSquared() != 0;

	// When sprinting, there's no lock movement (orbit) since we don't have
	// so fast lateral movement animations
	bool lockedMovement = is_locked_on && !is_sprinting;

	// Discard locked moves going sides+backwards
	lockedMovement &= !(fabsf(moveDir.x) > 0.f && moveDir.z < 0.f);
	
	if (!moving && !lockedMovement) {
		rot_factor = 0.0f;
		return moveDir;
	}

	// Modify player direction with camera rotation
	CEntity* e_camera = EngineRender.getActiveCamera();
	if (!e_camera)
		return moveDir;
	TCompCamera* c_camera = e_camera->get<TCompCamera>();
	TCompTransform* c_player_trans = h_transform;
	TCompTransform* c_locked_trans = h_locked_transform;

	VEC3 front;
	if (lockedMovement) {
		// Move towards locked enemy
		front = c_locked_trans->getPosition() - c_player_trans->getPosition();
		front.Normalize();
	}
	else {
		rot_factor = std::clamp((moveDir / moveDir.LengthSquared()).x, -1.0f, 1.0f);
		// Move where the camera is facing
		front = c_camera->getForward();
	}

	QUAT R = QUAT::CreateFromAxisAngle(VEC3::Up, vectorToYaw(front));
	moveDir = VEC3::Transform(moveDir, R);
	moveDir.Normalize();

	current_rot = c_player_trans->getRotation();

	// Rotate lerping amount
	if (lockedMovement) {
		last_dir = VEC3::Zero;
		new_rot = QUAT::CreateFromAxisAngle(VEC3(0, 1, 0), atan2(front.x, front.z));
		c_player_trans->setRotation(dampQUAT(current_rot, new_rot, 10.0f, Time.delta));
	}
	else {
		new_rot = QUAT::CreateFromAxisAngle(VEC3(0, 1, 0), atan2(moveDir.x, moveDir.z));
		float fwdDot = c_player_trans->getForward().Dot(moveDir);

		TCompFSM* fsm = get<TCompFSM>();
		fsm::CStateBaseLogic* currState = (fsm::CStateBaseLogic*)fsm->getCurrentState();
		float time_in_anim = fsm->getCtx().getTimeInState();

		if (fwdDot < -0.75f && is_sprinting && time_in_anim >= 1.f) {
			setVariable("is_turn_sprint", true);
		}
		else {
			c_player_trans->setRotation(dampQUAT(current_rot, new_rot, 15.0f, Time.delta));
		}
	}

	assert(!isnan(moveDir.x));
	if (isnan(moveDir.x)) { return VEC3(); }

	return moveDir;
}

void TCompPlayerController::move(float dt)
{
	TCompGameManager* c_gm = GameManager->get<TCompGameManager>();
	if (!currentPath.empty()) {
		return;
	}

	bool isShooterCamerEnabled = isCameraEnabled("camera_shooter");

	PlayerInteraction.checkInteractions();

	calcMoveDirection();

	is_turn_sprint = std::get<bool>(getVariable("is_turn_sprint"));

	if (!isShooterCamerEnabled && !is_turn_sprint && move_dir == VEC3::Zero && last_dir != VEC3::Zero) {
		// to complete the rotation
		TCompTransform* transform = h_transform;
		QUAT current_rot = transform->getRotation();
		QUAT new_rot = QUAT::CreateFromAxisAngle(VEC3(0, 1, 0), atan2(last_dir.x, last_dir.z));
		transform->setRotation(dampQUAT(current_rot, new_rot, 10.0f, dt));
	}
	else if (!is_locked_on) {
		last_dir = move_dir;
	}

	if(PlayerInteraction.isEnergyWallActive())
		return;

	bool isIteractCameraEnabled = isCameraEnabled("camera_interact");
	if (is_moving && isIteractCameraEnabled && !c_gm->isInCinematic())
		blendCamera("camera_follow", 1.0f, &interpolators::linearInterpolator);
	
	// ----------------
	// Speed management
	// ----------------

	// Set movement pose

	if (PlayerInput[VK_MENU].isPressed() || (inWater() && floorDistance > 0.55f)) {
		target_speed = isShooterCamerEnabled ? 0.f : walk_speed;
	}
	else {
		target_speed = move_speed;

		// to continue sprinting after recover the max stamine
		// if the sprint key is still pressed, can start sprinting 
		if (can_continue_sprinting && hasMaxStamina()) {
			can_sprint = true;
		}

		// to start sprinting
		if ((can_sprint && is_moving && !is_sprinting)) {
			can_recover_stamina = false;
			is_sprinting = true;
		}

		// to stop sprinting
		if ((!can_sprint || !is_moving) && is_sprinting) {
			can_recover_stamina = true;
			is_sprinting = false;
			target_speed = move_speed;
		}

		if (is_sprinting && !isShooterCamerEnabled) {
			sprint(dt);
		}
	}

	// Slower if... 
	if (is_locked_on) {
		// back
		if(fabsf(raw_dir.x) < 0.1f && raw_dir.z < 0.f)
			target_speed *= 0.7f;
		// or sides
		else if(fabsf(raw_dir.x) > 0.1f && raw_dir.z < 0.1f)
			target_speed *= 0.9f;
	}

	if (checkDashInput()) {
		setDashAnim();
	}

	// Manage on stop running/sprinting animations
	if (!is_turn_sprint && !is_moving && (PlayerInput["sprint"].wasPressed() || PlayerInput["sprint"].timeSinceReleased() < .2f) && moving_timer > 0.75f) {
		moving_timer = 0.f;
		current_speed = 0.0f;
		setVariable("is_stopping_sprint", true);
	}

	if (is_moving)
		moving_timer += dt;

	float lerp_factor = is_sprinting || target_speed == walk_speed ? 8.5f : 2.5f;
	current_speed = damp<float>(current_speed, target_speed * speed_multiplier_move, lerp_factor, dt);
	movePhysics(move_dir * current_speed * dt, dt);

	if (!isShooterCamerEnabled && !block_attacks)
	{
		if (PlayerInput["attack_regular"].getsPressed() && hasStamina()) {

			// Add combo here in case we are NOT attacking!
			TCompFSM* fsm = get<TCompFSM>();
			fsm::CStateBaseLogic* currState = (fsm::CStateBaseLogic*)fsm->getCurrentState();
			
			int attackingHeavy = std::get<int>(getVariable("is_attacking_heavy"));
			if (attackingHeavy != 0 && !currState->inRecoveryFrames(fsm->getCtx())) {
				return;
			}

			if (is_sprinting) {
				is_sprint_attack = true;
				setVariable("is_sprint_regular_attack", true);
			}
			else if (currState->name != "Attack" && currState->name != "Dash")
				setVariable("attack_regular", getNextAttack(true));
		}

		if (PlayerInput["attack_heavy"].isPressed() && hasStamina()) {

			if (is_sprinting) {
				is_sprint_attack = true;
				setVariable("is_sprint_strong_attack", true);
			}
			else {
				heavy_attack_timer += dt;
				// automatic attack when reaching max time
				if (heavy_attack_timer > 0.35f) {
					heavy_attack_timer = 0.f;
					setVariable("is_attacking_heavy", 2);
				}
			}
		}

		if (PlayerInput["attack_heavy"].getsReleased() && hasStamina()) {

			setVariable("is_attacking_heavy", 1);
			heavy_attack_timer = 0.f;
		}

		/*if (PlayerInput["parry"].getsPressed() && hasStamina()) {
			setVariable("in_parry", true);
		}*/

		if (PlayerInput["heal"].getsPressed()) {
			CEntity* owner = getEntity();
			TCompHeal* heal_comp = owner->get<TCompHeal>();
			bool canHeal = hasEnoughWarpEnergy(heal_comp->getWarpConsumption());
			if (canHeal) {
				setVariable("is_healing", true); 
				TCompTransform* transform = h_transform;
				PawnUtils::spawnHealParticles(transform->getPosition(), "player");
			}
		}
	}

	bool isPlayingState = CEngine::get().getModuleManager().inGamestate("playing");
	if (isPlayingState && !is_sprint_attack && !is_falling && manageFalling(current_speed, dt)) {
		is_falling = true;
		setVariable("is_falling", true);
	}

	TCompGameManager* gm = GameManager->get<TCompGameManager>();
	if (/*gm->isGardKilled() && */PlayerInput["cast_area"].getsPressed()) {

		castArea(isShooterCamerEnabled);
	}
}

void TCompPlayerController::setFloorMaterialInfo(const CPhysicalMaterial* material, VEC3 hitPos)
{
	lastFloorMat = material;

	if (!inWater())
		return;

	VEC3 waterPos = VEC3(0,-1e6, 0);

	{
		// Fill waters list
		if (water_entities.empty()) {
			water_entities = getEntitiesByString("water", [](CHandle h) {
				// If has collider, it's a water floor
				CEntity* e = h;
				TCompCollider* collider = e->get<TCompCollider>();
				TCompCompute* compute_shader = e->get<TCompCompute>();
				return !collider && !compute_shader;
			});
		}

		CEntity* closest = nullptr;
		float dist = 1e8f;

		// Search for closer water
		for (auto h : water_entities) {
			CEntity* e = h;
			float currentDist = VEC3::DistanceSquared(e->getPosition(), getEntity()->getPosition());
			if (currentDist >= dist)
				continue;
			closest = h;
			dist = currentDist;
		}

		if(closest)
			waterPos = closest->getPosition();
	}
	
	/*CTransform t;
	t.setPosition(waterPos);
	CEntity* splash = spawn("data/particles/water_splash.json", t);
	EngineLua.executeScript("destroyEntity('" + std::string(splash->getName()) + "')", 1.f);*/

	// The raycast adds 1m
	floorDistance = fabsf(waterPos.y - hitPos.y);
	// printFloat("Distance", floorDistance);
}

bool TCompPlayerController::inWater()
{
	return lastFloorMat && std::strcmp(lastFloorMat->getName(), "Water") == 0;
}

bool TCompPlayerController::isMoving(bool only_pressing)
{
	if (!only_pressing)
		return is_moving;

	// Check if player is pressing movement keys
	
	bool pressed = PlayerInput["move_keyboard"].isPressed();
	pressed |= PlayerInput["move_forward"].isPressed();
	pressed |= PlayerInput["move_back"].isPressed();
	pressed |= PlayerInput["move_left"].isPressed();
	pressed |= PlayerInput["move_right"].isPressed();

	return pressed;
}

void TCompPlayerController::castArea(bool isShooterCamerEnabled)
{
	CEntity* owner = getEntity();
	TCompPlayerController* controller = get<TCompPlayerController>();
	TCompAreaDelay* area_delay_comp = get<TCompAreaDelay>();

	CHandle t_locked = getLockedTransform();
	bool isLockedOn = t_locked.isValid();
	bool casted = area_delay_comp->startAreaDelay(isLockedOn ? t_locked : CHandle());

	if (casted) {
		if (!isShooterCamerEnabled)
			PawnUtils::playAction(owner, "SimpleInteract", 0.1f, 0.25f);
		else {
			PawnUtils::playAction(owner, "AreaDelay_Throw", 0.1f, 0.25f);
			TCompTransform* transform = h_transform;
			EngineAudio.postEvent("CHA/Eon/AT/Eon_Throw_AD_Ball", owner->getPosition());
		}
	}
	else if (isShooterCamerEnabled) {

		if (!area_delay_comp->isCasted())
		{
			area_delay_comp->destroyADBall(false);
		}

		PawnUtils::stopAction(getEntity(), "AreaDelay_Begin", 0.05f);
		blendCamera("camera_follow", 1.5f, &interpolators::cubicInOutInterpolator);
	}
}

VEC2 TCompPlayerController::getBlendSpeed()
{
	VEC2 blend_speed = VEC2::Zero;

	if (PlayerInteraction.isEnergyWallActive())
			return VEC2(0.f, 2.0f);

	bool inPath = currentPath.size() > 0;

	if (!is_moving && !inPath)
		return blend_speed;

	TCompCollider* c_collider = h_collider;
	bool stopped = c_collider->getLinearVelocity().Length() < 0.2f;

	float move_speed = stopped ? 0.f : current_speed;
	// ------------
	// When sprinting, there's no lock movement (orbit) since we don't have
	// so fast lateral movement animations
	bool lockedMovement = is_locked_on && !is_sprinting;
	// ------------

	if (inPath)
	{
		blend_speed = VEC2(0.f, currentPathSpeed);
	}
	else if (!lockedMovement)
	{
		blend_speed = VEC2(0.f, move_speed);
	}
	else 
	{
		TCompTransform* transform = h_transform;
		float dx = move_dir.Dot(transform->getRight());
		float dz = move_dir.Dot(transform->getForward());
		blend_speed = VEC2(dx, dz) * move_speed;
	}

	return blend_speed;
}

bool TCompPlayerController::hasMaxStamina()
{
	return static_cast<TCompStamina*>(h_player_stamina)->hasMaxStamina();
}

bool TCompPlayerController::hasStamina()
{
	return static_cast<TCompStamina*>(h_player_stamina)->hasStamina();
}

bool TCompPlayerController::checkDashInput()
{
	bool isShooterCameraEnabled = isCameraEnabled("camera_shooter");
	
#ifdef SAME_SPRINT_DASH_BTN
	input::TButton sprint_button = PlayerInput["sprint"];
	return !isShooterCamerEnabled && sprint_button.getsReleased() && hasStamina()
		&& sprint_button.prevTimeSincePressed() < dash_check_elapsed;
#else
	return !isShooterCameraEnabled && PlayerInput["dash"].getsPressed() && hasStamina();
#endif
}

bool TCompPlayerController::checkSprintInput()
{
	input::TButton sprint_button = PlayerInput["sprint"];

#ifdef SAME_SPRINT_DASH_BTN
	return !can_continue_sprinting && !is_sprinting && sprint_button.isPressed() &&
		sprint_button.prevTimeSincePressed() >= dash_check_elapsed;
#else
	return  !can_continue_sprinting && !is_sprinting && sprint_button.isPressed() && hasStamina();
#endif
}

int TCompPlayerController::getNextAttack(bool force_next)
{
	if (continue_attacking || force_next) {
		attack_count += 1;

		if (attack_count > max_combo_attacks)
			attack_count = 1;
	}
	return attack_count;
}

void TCompPlayerController::resetAttackCombo()
{
	attack_count = 0;
	continue_attacking = false;
	continue_attacking2 = false;
	can_recover_stamina = true;
}

bool TCompPlayerController::hasEnoughWarpEnergy(int value)
{
	return static_cast<TCompWarpEnergy*>(h_player_warp_energy)->hasWarpEnergy(value);
}

void TCompPlayerController::setVariable(const std::string& name, fsm::TVariableValue value)
{
	TMsgFSMVariable msg;
	msg.name = name;
	msg.value = value;
	CHandle(this).getOwner().sendMsg(msg);
}

fsm::TVariableValue TCompPlayerController::getVariable(const std::string& name)
{
	TCompFSM* fsm = get<TCompFSM>();
	return fsm->getCtx().getVariableValue(name);
}

void TCompPlayerController::resetMoveTimer()
{
	// Don't do stop run animation when attack is finished
	moving_timer = 0.f;
}

void TCompPlayerController::setDashAnim()
{
	TCompTransform* transform = h_transform;
	is_dashing = true;
	is_sprinting = false;
	if (!is_locked_on)
		setVariable("is_dashing", true);
	else {
		float dir_z = move_dir.Dot(transform->getForward());
		float dir_x = move_dir.Dot(transform->getRight());

		if (dir_z == 0.f && dir_x == 0.f)
			setVariable("is_dashing", true);
			//setVariable("dash_vertical", 1.f);
		else if (std::abs(dir_z) > std::abs(dir_x))
			dir_z > 0.f ? setVariable("is_dashing", true) 
			: setVariable("dash_vertical", dir_z);
		else
			setVariable("dash_horizontal", dir_x);
	}
}

bool TCompPlayerController::isCameraEnabled(const std::string& cameraName)
{
	CEntity* e_camera = getEntityByName(cameraName);
	if (!e_camera) return false;
	IGameplayCamera* active_cam = IGameplayCamera::fromCamera(e_camera);
	return active_cam->enabled;
}

void TCompPlayerController::calcMoveDirection()
{
	move_dir = getMoveDirection(is_moving);
}

bool TCompPlayerController::moveTo(VEC3 position, float speed, float acceptance, std::function<void()> cb)
{
	pathSpeed = speed;
	pathCallback = cb;
	pathAcceptanceDistance = acceptance;
	pathIndex = 0;
	currentPath.clear();
	EngineNavMesh.getPath(getEntity()->getPosition(), position, currentPath);
	return currentPath.empty();
}

void TCompPlayerController::setPathSpeed(float speed)
{
	pathSpeed = speed;
}

void TCompPlayerController::updatePath(float dt)
{
	if (pathIndex < currentPath.size())
	{
		TCompTransform* transform = h_transform;
		VEC3 target = currentPath[pathIndex];
		VEC3 origin = transform->getPosition();

		float lerp_factor = 3.f;
		currentPathSpeed = damp<float>(currentPathSpeed, pathSpeed, lerp_factor, dt);

		float dist = VEC3::Distance(origin, target);
			
		move_dir = normVEC3(target - origin);
		current_rot = transform->getRotation();
		new_rot = QUAT::CreateFromAxisAngle(VEC3(0, 1, 0), atan2(move_dir.x, move_dir.z));
		transform->setRotation(dampQUAT(current_rot, new_rot, 5.0f, dt));
		move_dir *= currentPathSpeed * dt;
		
		rot_factor = 0.f;
		last_dir = move_dir;
		movePhysics(move_dir, dt);

		// Detect end
		origin += move_dir;
		dist = VEC2::Distance(VEC2(origin.x, origin.z), VEC2(target.x, target.z));
		printFloat("distace to waypoint", dist);

		if (dist < pathAcceptanceDistance) {
			pathIndex++;
		}
	}
	else 
	{
		pathIndex = 0;
		currentPath.clear();

		if (pathCallback)
		{
			pathCallback();
			pathCallback = nullptr;
		}
	}
}

void TCompPlayerController::blendCamera(const std::string& cameraName, float blendTime, const interpolators::IInterpolator* interpolator)
{
	IGameplayCamera* new_cam = IGameplayCamera::fromCamera(getEntityByName(cameraName));
	assert(new_cam);
	IGameplayCamera* last_cam = IGameplayCamera::fromCamera();
	
	if(new_cam && last_cam)
		new_cam->deltasFromCamera(*last_cam);

	// enable new camera and disable not used cameras
	TCompGameManager* gm = GameManager->get<TCompGameManager>();
	gm->enableCamera(cameraName);

	// mix new camera
	CameraMixer.blendCamera(cameraName, blendTime, interpolator);
}

void TCompPlayerController::manageAimCamera()
{
	if (_aimLocked)
		return;

	bool canAim = !is_aiming && !isAlive("Plasma_Ball");

	TCompFSM* fsm = get<TCompFSM>();
	assert(fsm);
	fsm::CStateBaseLogic* currState = (fsm::CStateBaseLogic*)fsm->getCurrentState();
	if (currState)
		canAim &= currState->isBlendSpace();

	if (PlayerInput["aim"].getsPressed() && canAim)
	{
		is_aiming = true;
		TCompAreaDelay* area_delay_comp = get<TCompAreaDelay>();
		assert(area_delay_comp);

		if (hasEnoughWarpEnergy(area_delay_comp->getWarpConsumption()) &&
			!area_delay_comp->isActive())
		{
			blendCamera("camera_shooter", 0.5f, &interpolators::quadInOutInterpolator);
			PawnUtils::playAction(getEntity(), "AreaDelay_Begin", 0.2f);
			TCompTransform* transform = h_transform;
			EngineAudio.postEvent("CHA/Eon/AT/Eon_Begin_AD_Ball", transform->getPosition());
			// Use as AD Back Cycle
			PawnUtils::playCycle(getEntity(), "AreaDelay_Idle", 0.3f);
			CHandle h = spawn("data/prefabs/plasma_ball.json", CTransform());
			area_delay_comp->setADBall(h);
		}
	}
	
	if (PlayerInput["aim"].getsReleased() && is_aiming)
	{
		TCompAreaDelay* area_delay_comp = get<TCompAreaDelay>();
		assert(area_delay_comp);
		if (!area_delay_comp->isCasted())
		{
			area_delay_comp->destroyADBall(false);
			onStopAiming(TMsgStopAiming({ 0.5f }));
		}
	}
}

void TCompPlayerController::removeLockOn(bool recenter)
{
	CEntity* e_camera_follow = getEntityByName("camera_follow");
	TCompCameraFollow* c_camera_follow = e_camera_follow->get<TCompCameraFollow>();
	
	if (is_locked_on) {
		c_camera_follow->setLockedEntity(CHandle());
	}
	else if(recenter){
		c_camera_follow->must_recenter = true;
	}
	is_locked_on = false;
	h_locked_transform = CHandle();
}

void TCompPlayerController::manageLockOn()
{
	if (is_locked_on) {
		removeLockOn();
		return;
	}

	CEntity* e_camera_follow = getEntityByName("camera_follow");
	TCompCameraFollow* c_camera_follow = e_camera_follow->get<TCompCameraFollow>();
	TCompTransform* camera_trans = e_camera_follow->get<TCompTransform>();

	TCompTransform* c_transform = h_transform;
	TCompCollider* c_collider = h_collider;
	VEC3 player_pos = c_transform->getPosition();
	VHandles objects_in_area;

	// If anyone inside 
	if (!EnginePhysics.overlapSphere(player_pos, sight_radius, objects_in_area, CModulePhysics::FilterGroup::Enemy)) {
		c_camera_follow->reset();
		return;
	}

	float min_distance = 100000.0f;
	float max_angle = -1.f;
	float max_weight = -1.f;
	CEntity* h_locked_entity = nullptr;

	for (CHandle collider_handle : objects_in_area) {

		CHandle h_owner = collider_handle.getOwner();
		CEntity* entity_object = h_owner;

		if (!entity_object)
			continue;

		CHandle h_health = entity_object->get<TCompHealth>();
		TCompTransform* locked_trans = entity_object->get<TCompTransform>();

		if (!locked_trans || !h_health.isValid() || static_cast<TCompHealth*>(h_health)->isDead())
			continue;

		TCompHierarchy* c_hierarchy = entity_object->get<TCompHierarchy>();

		if (c_hierarchy) {
			entity_object = c_hierarchy->getRootEntity(entity_object);
		}

		// discard if it isn't in player's sight
		if (!PawnUtils::isInsideCone(e_camera_follow, h_owner, fov_rad, sight_radius))
			continue;

		VEC3 locked_pos = locked_trans->getPosition();
		VEC3 cameraFwd = camera_trans->getForward();
		VEC3 dirToEnemy = normVEC3(locked_pos - camera_trans->getPosition());

		// Check distance to player
		VEC3 dir = locked_pos - player_pos;
		float distance = dir.Length();
		VEC3 base_ray_pos = player_pos + VEC3::Up;

		bool can_see_target = canSeeTarget(camera_trans, entity_object);
		if (!can_see_target) continue;

		float distance2d = (VEC2(locked_pos.x, locked_pos.z) - VEC2(player_pos.x, player_pos.z)).Length();
		if (distance2d < 5.f && std::fabs(player_pos.y - locked_pos.y) > 2.f)
			continue;

		// Check alignment with camera
		float dot = cameraFwd.Dot(dirToEnemy);
		// Check height diff
		float h_diff = clampf(fabsf(locked_pos.y - player_pos.y), 0.f, 20.f);

		// Compute weight using all variables
		float weight = 0.f;
		weight += w_distance * (1.f - distance / sight_radius);
		weight += w_alignment * dot;
		weight += w_height_diff * (1.f - mapToRange(0, 20, 0, 1, h_diff));

		if (weight > max_weight) {
			h_locked_entity = entity_object;
			max_weight = weight;
		}
	}

	if (h_locked_entity) {
		TCompBT* e_bt = h_locked_entity->get<TCompBT>();
		if (e_bt && !e_bt->isDying()) {
			is_locked_on = true;
			h_locked_transform = h_locked_entity->get<TCompTransform>();
			c_camera_follow->setLockedEntity(h_locked_entity);
		}
	}
	else {
		c_camera_follow->reset();
	}
}

// flick the mouse in a direction towards an enemy to change targets without unlocking
void TCompPlayerController::changeLockedTarget(VEC2& dir)
{
	CEntity* e_camera_follow = getEntityByName("camera_follow");
	TCompCameraFollow* c_camera_follow = e_camera_follow->get<TCompCameraFollow>();

	TCompTransform* camera_trans = e_camera_follow->get<TCompTransform>();
	VHandles objects_in_area;

	TCompTransform* c_transform = h_transform;
	TCompCollider* c_collider = h_collider;
	VEC3 player_pos = c_transform->getPosition();

	if (!EnginePhysics.overlapSphere(camera_trans->getPosition(), sight_radius, objects_in_area, CModulePhysics::FilterGroup::Enemy))
		return;

	float min_distance = 100000.0f;
	VEC3 front = camera_trans->getForward();
	CEntity* h_locked_entity = nullptr;
	TCompTransform c_locked_trans = *(static_cast<TCompTransform*>(h_locked_transform));

	QUAT new_rot = QUAT::CreateFromAxisAngle(VEC3(0, 1, 0), atan2(front.x, front.z));
	c_locked_trans.setRotation(new_rot);

	// check if the entity has transform 
	// if not, discard it
	VEC3 cameraFwd = camera_trans->getForward();

	float upDot2D = VEC2::UnitY.Dot(dir);
	float rgtDot2D = VEC2::UnitX.Dot(dir);

	for (CHandle collider_handle : objects_in_area) {

		CHandle h_owner = collider_handle.getOwner();
		CEntity* entity_object = h_owner;

		if (!entity_object)
			continue;

		CHandle h_health = entity_object->get<TCompHealth>();

		if (!h_health.isValid())
			continue;

		if (static_cast<TCompHealth*>(h_health)->isDead())
			continue;

		TCompHierarchy* c_hierarchy = entity_object->get<TCompHierarchy>();

		if (c_hierarchy) {
			entity_object = c_hierarchy->getRootEntity(entity_object);
		}

		TCompTransform* locked_trans = entity_object->get<TCompTransform>();
		if (!locked_trans)
			continue;

		if (static_cast<TCompTransform*>(h_locked_transform) == locked_trans)
			continue;

		if (c_camera_follow->checkUnlockDistance(entity_object))
			continue;

		TCompTransform copy_locked_trans = *locked_trans;

		// If it's inside player's cone
		if (!PawnUtils::isInsideCone(e_camera_follow, h_owner, (float)M_PI_2, sight_radius))
			continue;

		VEC3 locked_pos = copy_locked_trans.getPosition();
		VEC3 cameraFwd = camera_trans->getForward();
		VEC3 cameraRight = camera_trans->getRight();
		VEC3 dirToEnemy = locked_pos - c_locked_trans.getPosition();

		VEC3 dir = locked_pos - player_pos;
		float distance = dirToEnemy.Length();
		float distance2d = (VEC2(locked_pos.x, locked_pos.z) - VEC2(player_pos.x, player_pos.z)).Length();
		dirToEnemy.Normalize();

		VEC3 base_ray_pos = player_pos + VEC3::Up;

		bool can_see_target = canSeeTarget(camera_trans, entity_object);
		if (!can_see_target) continue;

		if (distance2d < 5.f && std::fabs(player_pos.y - locked_pos.y) > 2.f)
			continue;

		copy_locked_trans.setRotation(new_rot);

		float fwdDot = c_locked_trans.getForward().Dot(dirToEnemy);
		float rgtDot = c_locked_trans.getRight().Dot(dirToEnemy);
		
		VEC2 dirMouse = VEC2::Zero;
		// dbg("Mouse dir %f %f\n", dir.x, dir.y);
		if (std::abs(rgtDot2D) >= std::abs(upDot2D)) {

			if (rgtDot2D < 0.f) {
				dirMouse.x = -1.0f;
			}
			else {
				dirMouse.x = 1.0f;
			}
		}
		else if (upDot2D < 0.f) {
			dirMouse.y = -1.0f;
		}
		else {
			dirMouse.y = 1.0f;
		}

		if ((dirMouse.y != 0 && sameSign(fwdDot, dirMouse.y)) || (dirMouse.x != 0 && sameSign(rgtDot, dirMouse.x))) {

			if (distance < min_distance) {
				min_distance = distance;
				h_locked_entity = entity_object;
			}
		}
	}

	if (!h_locked_entity)
		return;

	if (is_locked_on) {
		removeLockOn();
	}

	if (h_locked_entity) {
		TCompBT* e_bt = h_locked_entity->get<TCompBT>();
		if (e_bt && !e_bt->isDying()) {
			is_locked_on = true;
			h_locked_transform = h_locked_entity->get<TCompTransform>();
			c_camera_follow->setLockedEntity(h_locked_entity);
		}
	}
}

void TCompPlayerController::lockOnToTarget(CHandle target)
{
	if (is_locked_on) {
		removeLockOn();
	}

	CEntity* e_camera_follow = getEntityByName("camera_follow");
	TCompCameraFollow* c_camera_follow = e_camera_follow->get<TCompCameraFollow>();
	TCompTransform* camera_trans = e_camera_follow->get<TCompTransform>();

	TCompTransform* c_transform = h_transform;
	TCompCollider* c_collider = h_collider;
	VEC3 player_pos = c_transform->getPosition();

	float min_distance = 100000.0f;
	float max_angle = -1.f;
	float max_weight = -1.f;
	CEntity* h_locked_entity = nullptr;


	CEntity* entity_object = target;

	TCompTransform* locked_trans = entity_object->get<TCompTransform>();

	if (!locked_trans)
		return;

	TCompHierarchy* c_hierarchy = entity_object->get<TCompHierarchy>();

	if (c_hierarchy) {
		entity_object = c_hierarchy->getRootEntity(entity_object);
	}

	// discard if it isn't in player's sight
	if (!PawnUtils::isInsideCone(e_camera_follow, target, fov_rad, sight_radius))
		return;

	VEC3 locked_pos = locked_trans->getPosition();
	VEC3 cameraFwd = camera_trans->getForward();
	VEC3 dirToEnemy = normVEC3(locked_pos - camera_trans->getPosition());

	// Check distance to player
	VEC3 dir = locked_pos - player_pos;
	float distance = dir.Length();
	VEC3 base_ray_pos = player_pos + VEC3::Up;

	bool can_see_target = canSeeTarget(camera_trans, entity_object);
	//if (!can_see_target) return;

	float distance2d = (VEC2(locked_pos.x, locked_pos.z) - VEC2(player_pos.x, player_pos.z)).Length();
	if (distance2d < 5.f && std::fabs(player_pos.y - locked_pos.y) > 2.f)
		return;

	// Check alignment with camera
	float dot = cameraFwd.Dot(dirToEnemy);
	// Check height diff
	float h_diff = clampf(fabsf(locked_pos.y - player_pos.y), 0.f, 20.f);

	// Compute weight using all variables
	float weight = 0.f;
	weight += w_distance * (1.f - distance / sight_radius);
	weight += w_alignment * dot;
	weight += w_height_diff * (1.f - mapToRange(0, 20, 0, 1, h_diff));

	if (weight > max_weight) {
		h_locked_entity = entity_object;
		max_weight = weight;
	}

	if (h_locked_entity) {
		is_locked_on = true;
		h_locked_transform = h_locked_entity->get<TCompTransform>();
		c_camera_follow->setLockedEntity(h_locked_entity);
	}
	else {
		c_camera_follow->reset();
	}
}

bool TCompPlayerController::canSeeTarget(TCompTransform* camera_trans, CEntity* e_target)
{
	float maxDistance = 50.f;

	VEC3 lock_target_pos = VEC3::Zero;
	TCompLockTarget* lockTarget = e_target->get<TCompLockTarget>();

	float finalHeight = 1.3f;
	if (!lockTarget)
		false;

	lock_target_pos = lockTarget->getPosition();

	float lockHeight = lockTarget->getHeight();

	if (lockHeight != -1.f)
		finalHeight = lockHeight;

	VEC3 dir_from_camera = lock_target_pos - camera_trans->getPosition();

	VHandles colliders;
	physx::PxU32 layerMask = CModulePhysics::FilterGroup::Scenario | CModulePhysics::FilterGroup::Enemy;

	bool is_ok = EnginePhysics.raycast(camera_trans->getPosition(), normVEC3(dir_from_camera), maxDistance, colliders, layerMask, true);
	if (is_ok) {
		TCompCollider* c_collider = colliders[0];
		CEntity* e_firsthit = c_collider->getEntity();

		TCompBT* e_bt = e_firsthit->get<TCompBT>();

		if (!e_bt) return false;
	}

	return true;
}

void TCompPlayerController::sprint(float dt)
{
	TCompStamina* comp_stamina = h_player_stamina;

	if (comp_stamina->hasStamina(EAction::SPRINT)) {
		target_speed = sprint_speed;
		start_sprint_time +=	 dt;

		if (start_sprint_time >= sprint_redux_freq) {
			comp_stamina->reduceStamina(EAction::SPRINT);
			start_sprint_time = 0;
		}
	}
	else {
		target_speed = move_speed;
		can_recover_stamina = true;
		is_sprinting = false;
		// can continue sprinting? if the shift key is still pressed
		can_continue_sprinting = can_sprint;
		can_sprint = false;
	}
}

void TCompPlayerController::execEffectiveParry(const TMsgHit& msg)
{
	if (msg.hitByProjectile) {
		CEntity* e_proyectile = msg.h_striker;
		VEC3 origin = msg.position - msg.forward;
		VEC3 direction = -msg.forward;
		TaskUtils::spawnProjectile(origin, origin + direction * 100, msg.damage, true);

#ifdef _DEBUG
		std::vector<physx::PxRaycastHit> raycastHits;
		EnginePhysics.raycast(origin, direction, 200.f, raycastHits);
#endif
	}
	else {
		CHandle striker = msg.h_striker;
		TMsgEnemyStunned pmsg;
		assert(striker.isValid());
		striker.sendMsg(pmsg);
	}
}

/*
	Event callbacks
*/

void TCompPlayerController::onHit(const TMsgHit& msg)
{
	if (msg.hitByPlayer)
		return;

	TCompFSM* fsm = get<TCompFSM>();
	fsm::CStateBaseLogic* currState = (fsm::CStateBaseLogic*)fsm->getCurrentState();
	fsm::CContext& ctx = fsm->getCtx();
	bool onActiveFrames = currState->inActiveFrames(ctx);

	bool effectiveDash = is_dashing && onActiveFrames;

	bool onParry = (currState->name == "Parry");
	bool effectiveParry = onParry && onActiveFrames
		&& PawnUtils::isInsideCone(CHandle(this).getOwner(), msg.h_striker, deg2rad(90.f), 2.f);

	// Finally check if the attack landed on Eon
	if (effectiveParry) {
		ctx.setStateFinished(true);
		execEffectiveParry(msg);
	}
	else if (effectiveDash) { }
	else if (!GOD_MODE)
	{
		bool isCancellable = currState->isCancellableOnHit();

		if (currState->usesTimings())
			isCancellable &= currState->inStartupFrames(ctx);

		// Dash but no effective (no active frames)
		if (is_dashing) {
			TCompCollider* comp_collider = h_collider;
			static_cast<TCompCollider*>(h_collider)->stopFollowForceActor("dash");
			is_dashing = false;
		}

		if (isCancellable) {

			std::string var = "is_stunned";

			switch (msg.hitType) {
			case EHIT_TYPE::SWEEP:
				var = "is_stunned_strong"; 
				break;
			case EHIT_TYPE::SMASH:
				var = "is_smashed";
				break;
			}

			setVariable(var, true);
		}

		// Scale with ARMOR!
		TCompAttributes* attrs = get<TCompAttributes>();
		int dmg_scaled = attrs->computeDamageReceived(msg.damage);

		// Reduce health
		TMsgReduceHealth hmsg;
		hmsg.damage = dmg_scaled;
		hmsg.h_striker = msg.h_striker;
		hmsg.hitByPlayer = msg.hitByPlayer;
		hmsg.fromBack = msg.fromBack;
		hmsg.skip_blood = msg.hitType == EHIT_TYPE::GARD_BRANCH;
		getEntity()->sendMsg(hmsg);

		// Disable Aim mode
		if (is_aiming)
		{
			TCompAreaDelay* area_delay_comp = get<TCompAreaDelay>();
			assert(area_delay_comp);
			if (!area_delay_comp->isCasted())
			{
				area_delay_comp->destroyADBall(false);
				onStopAiming(TMsgStopAiming({ 0.5f }));
			}
		}

		// FMOD damage event
		TCompTransform* t = get<TCompTransform>();
		EngineAudio.postEvent("CHA/Eon/DMG/Eon_Receive_DMG", t->getPosition());
	}
}

void TCompPlayerController::onLockedTargetDied(const TMsgEnemyDied& msg)
{
	if (msg.h_enemy == h_locked_transform.getOwner())
	{	
		// unlock current target (enemy)
		removeLockOn();
		// auto-lock after death of the current target
		manageLockOn();
	}
}

// Sent by game manager to allow Eon to revive. Change the state to the revive state
void TCompPlayerController::onEonRevive(const TMsgEonRevive& msg)
{
	setVariable("is_death", true);
	removeLockOn();
}

void TCompPlayerController::onShrineRestore(const TMsgRestoreAll& msg)
{
	static_cast<TCompHealth*>(get<TCompHealth>())->fillHealth();
	static_cast<TCompStamina*>(get<TCompStamina>())->fillStamina();
	static_cast<TCompWarpEnergy*>(get<TCompWarpEnergy>())->fillWarpEnergy();
}

void TCompPlayerController::onTimeReversalStopped(const TMsgTimeReversalStoped& msg)
{
	is_dashing = false;
	reversal_start_timer = 0.0f;

	setVariable("in_time_reversal", false);

	setVariable("linear_velocity", msg.linear_velocity);
	setVariable("attack_regular", 0);

	TCompFSM* fsm = get<TCompFSM>();
	fsm->getCtx().forceState(msg.state_name, msg.time_in_anim);

	// Edge case: player is dead after time reversal
	TCompHealth* c_health = get<TCompHealth>();
	if (c_health->getHealth() <= 0) {
		TMsgEonHasDied msgEonDied;
		msgEonDied.h_sender = CHandle(this);
		GameManager->sendMsg(msgEonDied);
	}

	if (manageFalling(current_speed, Time.delta)) {
		is_falling = true;
		setVariable("is_falling", true);
	}
}

void TCompPlayerController::onStopAiming(const TMsgStopAiming& msg)
{
	is_aiming = false;
	blendCamera("camera_follow", msg.blendIn, &interpolators::cubicInOutInterpolator);
	PawnUtils::stopAction(getEntity(), "AreaDelay_Begin", 0.3f);
	PawnUtils::stopCycle(getEntity(), "AreaDelay_Idle", 0.3f);
}

void TCompPlayerController::onApplyAreaDelay(const TMsgApplyAreaDelay& msg)
{
	EngineAudio.postEvent("CHA/Eon/DMG/Eon_Receive_Area_Delay");

	const char* FMOD_EON_INSIDE_WARP_PARAMETER = "Eon_Inside_Warp";
	EngineAudio.setGlobalRTPC(FMOD_EON_INSIDE_WARP_PARAMETER, 1.f);
}

void TCompPlayerController::onRemoveAreaDelay(const TMsgRemoveAreaDelay& msg)
{
	const char* FMOD_EON_INSIDE_WARP_PARAMETER = "Eon_Inside_Warp";
	EngineAudio.setGlobalRTPC(FMOD_EON_INSIDE_WARP_PARAMETER, 0.f);
}

void TCompPlayerController::debugInMenu()
{
	ImGui::Text("Attack count: %d", attack_count);
	ImGui::Text("Move timer: %f", moving_timer);
	ImGui::Text("Current Speed: %f", current_speed);
	ImGui::Separator();
	ImGui::DragFloat("Walk Speed", &walk_speed, 0.1f, 1.f, 5.f);
	ImGui::DragFloat("Move Speed", &move_speed, 0.1f, 1.f, 10.f);
	ImGui::DragFloat("Path Speed", &pathSpeed, 0.1f, 1.f, 10.f);
	ImGui::DragFloat("Sprint Speed", &sprint_speed, 0.1f, 1.f, 10.f);
	ImGui::DragFloat("Cone Full Fov", &fov_rad, 0.01f, 0.0f, (float)M_PI);
	ImGui::DragFloat("Cone Length", &sight_radius, 0.1f, 0.1f, 250.f);
	ImGui::Separator();
	ImGui::Text("Lock on Weights");
	ImGui::DragFloat("Distance", &w_distance, 0.01f, 0.0f, 1.f);
	ImGui::DragFloat("Alignment", &w_alignment, 0.01f, 0.0f, 1.f);
	ImGui::DragFloat("Height difference", &w_height_diff, 0.01f, 0.0f, 1.f);
}

void TCompPlayerController::renderDebug()
{
	TCompTransform* my_transform = h_transform;

	VEC3 pos = my_transform->getPosition();
	auto* dl = ImGui::GetBackgroundDrawList();

	float width = (float)Render.getWidth();
	float height = (float)Render.getHeight();

#ifdef _DEBUG

	// Debug frames
	{
		TCompFSM* fsm = get<TCompFSM>();
		fsm::CStateBaseLogic* currState = (fsm::CStateBaseLogic*)fsm->getCurrentState();
		std::string frames = "";

		if (currState->inStartupFrames(fsm->getCtx())) frames = "STARTUP";
		else if (currState->inActiveFrames(fsm->getCtx())) frames = "ACTIVE";
		else if (currState->inRecoveryFrames(fsm->getCtx())) frames = "RECOVERY";

		drawText3D(my_transform->getPosition() + VEC3(0, 2.5, 0), Colors::Red, frames.c_str(), 40.f);
	}

	if (is_falling)
		drawText3D(my_transform->getPosition() + VEC3(0, 2, 0), Colors::Red, "FALLING", 40.f);

	pos += VEC3(0, 1, 0);

	if (GOD_MODE)
		drawText3D(pos, Colors::ShinyBlue, "GOD", 30.f);

	TCompAttributes* attrs = get<TCompAttributes>();
	if (attrs) {

		float firstH = 150.f;

		attrs->forEach([&firstH](const std::string name, unsigned short int value) {
			std::string format = name + ": " + std::to_string(value);
			drawText2D(VEC2(Render.getWidth() - 150.f, Render.getHeight() - firstH), Colors::White,
				format.c_str(), false, false);
			firstH -= 25.f;
		});
	}
#endif
}