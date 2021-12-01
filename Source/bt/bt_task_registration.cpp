#include "mcv_platform.h"
#include "bt_parser.h"
#include "task_utils.h"
#include "bt.h"
#include "engine.h"
#include "bt_task.h"
#include "modules/module_physics.h"
#include "ui/ui_module.h"
#include "lua/module_scripting.h"
#include "audio/module_audio.h"
#include "navmesh/module_navmesh.h"
#include "entity/entity_parser.h"
#include "components/common/comp_render.h"
#include "components/common/comp_name.h"
#include "components/common/comp_transform.h"
#include "components/common/comp_parent.h"
#include "components/controllers/comp_ai_controller_base.h"
#include "components/controllers/comp_force_receiver.h"
#include "components/controllers/pawn_utils.h"
#include "components/abilities/comp_heal.h"
#include "components/cameras/comp_camera_shake.h"
#include "components/projectiles/comp_gard_branch.h"
#include "components/ai/comp_ai_time_reversal.h"
#include "components/render/comp_emissive_modifier.h"
#include "components/ai/comp_bt.h"
#include "components/stats/comp_health.h"
#include "skeleton/comp_skel_lookat.h"
#include "skeleton/comp_attached_to_bone.h"
#include "components/projectiles/comp_cygnus_beam.h"
#include "components/controllers/comp_player_controller.h"
#include "components/common/comp_buffers.h"
#include "../bin/data/shaders/constants_particles.h"

#define PLAY_CINEMATICS true

/*
 *	Declare and implement all tasks here
 *	The macros at the bottom will register each of them
 */

#pragma region Movement Tasks
class CBTTaskChooseWaypoint : public IBTTask
{
	bool is_random = true;
	std::vector<VEC3> waypoints = {};

public:
	void init() override {
		
		// If random is set to 0, choose specific waypoints from the AI Controller
		if (!number_field[1]) {
			is_random = false; 
		}
	}
	
	void onEnter(CBTContext& ctx) override {
		
		if(ctx.getBlackboard()->isValid("randomPatrol"))
			is_random = ctx.getBlackboard()->getValue<bool>("randomPatrol");

		if (!is_random) {
			TCompAIControllerBase* h_ai_controller = ctx.getComponent<TCompAIControllerBase>();
			waypoints = h_ai_controller->getWaypoints();
			if (waypoints.empty()) {
				TCompTransform* h_trans = ctx.getComponent<TCompTransform>();
				waypoints.push_back(h_trans->getPosition());
			}

			if (!ctx.isNodeVariableValid(name, "curr_wp_index")) {
				ctx.setNodeVariable(name, "curr_wp_index", -1);
			}
		}

	}
	
	EBTNodeResult executeTask(CBTContext& ctx, float dt) {

		TCompTransform* h_trans = ctx.getComponent<TCompTransform>();
		VEC3 wp;

		if (is_random) {
			// Generate waypoint with range number_field meters around current position
			wp = EngineNavMesh.getRandomPointAroundCircle(h_trans->getPosition(), number_field[0]);
		}
		else {
			// Get the waypoints array from the current enemy's AI controller
			TCompAIControllerBase* h_ai_controller = ctx.getComponent<TCompAIControllerBase>();
			waypoints = h_ai_controller->getWaypoints();
			
			// If it is not random get the current index
			int curr_index = ctx.getNodeVariable<int>(name, "curr_wp_index");

			// If the current index is -1, start from the beginning
			if (curr_index == -1) {
				curr_index = 0;
				ctx.setNodeVariable(name, "curr_wp_index", curr_index);
			}
			else {

				// Otherwise, start again (if it has reached the end) or go to the next position
				curr_index++;
				if (curr_index > (waypoints.size() - 1)) {
					curr_index = 0;
					ctx.setNodeVariable(name, "curr_wp_index", curr_index);
				}
				else {
					ctx.setNodeVariable(name, "curr_wp_index", curr_index);
				}
			}

			wp = waypoints[curr_index];
		}

		bool is_ok = ctx.getBlackboard()->setValue<VEC3>(string_field, wp);

		if (is_ok) {
			return EBTNodeResult::SUCCEEDED;
		}
		else {
			fatal("Undeclared blackboard variable \"%s\"", string_field);
			return EBTNodeResult::FAILED;
		}
	}
};

class CBTTaskTargetPlayer : public IBTTask
{
public:
	EBTNodeResult executeTask(CBTContext& ctx, float dt) {

		CEntity* e_player = getPlayer();
		assert(e_player);
		if (!e_player)
			return EBTNodeResult::FAILED;

		bool is_ok = ctx.getBlackboard()->setValue<CHandle>(string_field, e_player);
		if (is_ok) {
			return EBTNodeResult::SUCCEEDED;
		}
		else {
			fatal("Undeclared blackboard variable \"%s\"", string_field);
			return EBTNodeResult::FAILED;
		}
	}
};


class CBTTaskTargetNearPlayer : public IBTTask
{
private:
	float dist_to_target;
	int relative_pos = 1;						// Position relative to Eon: 1 = Front, 2 = Right, 3 = Back, 4 = Left
	std::string target_pos_name;

public:
	void init() override {
		// Load generic parameters
		dist_to_target = number_field[0];
		relative_pos = clamp((int)number_field[1], 1, 4);
		target_pos_name = string_field;
	}
public:
	EBTNodeResult executeTask(CBTContext& ctx, float dt) {

		CEntity* e_player = getPlayer();
		assert(e_player);
		if (!e_player)
			return EBTNodeResult::FAILED;

		TCompTransform* c_player = e_player->get<TCompTransform>();

		// Get offset vector relative to the player's position
		VEC3 offset_vector;
		switch (relative_pos) {
		case 1: offset_vector = c_player->getForward(); break;					// 1 = Front
		case 2: offset_vector = c_player->getRight(); break;						// 2 = Right
		case 3: offset_vector = c_player->getForward() * (-1); break;		// 3 = Back
		case 4: offset_vector = c_player->getRight() * (-1); break;			// 4 = Left
		default: offset_vector = c_player->getForward();
		}

		// Multiply the offset vector by the distance, minimum 2m
		VEC3 offset_pos = offset_vector * std::max(dist_to_target, 2.0f);
		VEC3 final_pos = c_player->getPosition() + offset_pos;

		// Store the position in the BB key
		bool is_ok = ctx.getBlackboard()->setValue<VEC3>(string_field, final_pos);

		if (is_ok) {
			return EBTNodeResult::SUCCEEDED;
		}
		else {
			fatal("Undeclared blackboard variable \"%s\"", string_field);
			return EBTNodeResult::FAILED;
		}
	}
};

class CBTTaskMoveToPosition : public IBTTask
{
private:
	const char* PATH_KEY = "path";
	float rotate_speed;
	float acceptance_dist;
	float max_accept_dist;

public:
	void init() override {
		// Load generic parameters
		move_speed = number_field[0];
		rotate_speed = number_field[1];
		acceptance_dist = number_field[2];
		max_accept_dist = acceptance_dist * 6;
	}

	void onEnter(CBTContext& ctx) override {
		TCompTransform* trans = ctx.getComponent<TCompTransform>();
		VEC3 target = ctx.getBlackboard()->getValue<VEC3>(string_field);
		
		// Store the precalculated path from the current position to the target
		TNavPath new_path;
		EngineNavMesh.getPath(trans->getPosition(), target, new_path);
		ctx.setNodeVariable(name, PATH_KEY, new_path);
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		// Move towards VEC3 position stored in the BB entry specified by the node in OWLbt (string_field)

		// if there is no path it must fail
		TNavPath cur_path = ctx.getNodeVariable<TNavPath>(name, PATH_KEY);
		if (cur_path.empty()) { return EBTNodeResult::FAILED; }

		EBTNodeResult res = navigateToPosition(ctx, move_speed, rotate_speed, acceptance_dist, max_accept_dist, dt);

		// If navigation has succeeded, reset/clean task
		if (res == EBTNodeResult::SUCCEEDED) {
			cleanTask(ctx);
		}

		return res;
	}

	void cleanTask(CBTContext& ctx) override {
		ctx.setNodeVariable(name, "target_index", 0);			// reset the next position of the path vector
		ctx.setNodeVariable(name, PATH_KEY, TNavPath()); // clear the path vector	
	}
};

class CBTTaskMoveToEntity : public IBTTask
{
private:
	const char* TARGET_IDX_KEY = "target_index";
	const char* PATH_KEY = "path";

	
	float rotate_speed;
	float acceptance_dist;

public:
	void init() override {
		move_speed = number_field[0];
		rotate_speed = number_field[1];
		acceptance_dist = number_field[2];
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		// Move towards entity handle stored in the BB entry specified by the node in OWLbt (string_field)

		TCompTransform* h_trans = ctx.getComponent<TCompTransform>();

		CHandle h_target = getPlayer();
		CEntity* e_target = h_target;
		VEC3 target = e_target->getPosition();
		TCompTransform* t_player = e_target->get<TCompTransform>();

		float cur_dist = VEC3::Distance(h_trans->getPosition(), target);

		// If the enemy is far from Eon or there is an obstacle, keep chasing Eon
		if (cur_dist > acceptance_dist || TaskUtils::hasObstaclesToEon(h_trans, t_player)) {

			TNavPath path;
			EngineNavMesh.getPath(h_trans->getPosition(), target, path);

			// No possible path
			if (path.empty() || path.size() == 1) { return EBTNodeResult::FAILED; }

			// Movement has not reached target

			TCompAIControllerBase* h_controller = ctx.getComponent<TCompAIControllerBase>();
			// Rotate to face target
			TaskUtils::rotateToFace(h_trans, path[1], rotate_speed, dt);

			// Advance forward
			TaskUtils::moveForward(ctx, move_speed, dt);

			return EBTNodeResult::IN_PROGRESS;
		}
		else {
			// Movement has reached target
			cleanTask(ctx);
			return EBTNodeResult::SUCCEEDED;
		}
	}

	void cleanTask(CBTContext& ctx) override {
		ctx.setNodeVariable(name, TARGET_IDX_KEY, 0);			// reset the next position of the path vector
		ctx.setNodeVariable(name, PATH_KEY, TNavPath()); // clear the path vector	
	}
};

class CBTTaskCombatMovement : public IBTTask
{
private:
	const char* DT_ACUM_KEY = "dt_acum";
	const char* PATH_KEY = "path";
	const char* TARGET_IDX_KEY = "target_index";

	const float MIN_RAND_FACTOR = 0.5f;
	const float MAX_RAND_FACTOR = 1.5f;

	float range;
	
	float rotation_speed;
	float period;

public:
	void init() override {
		move_speed = number_field[0];
		rotation_speed = number_field[1];
		range = number_field[2];
		period = number_field[3];
	}

	void onEnter(CBTContext& ctx) override {
		ctx.getBlackboard()->setValue("enableCombatMovement", true);
		ctx.setNodeVariable(name, DT_ACUM_KEY, period);
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {

		bool should_combat_move = ctx.getBlackboard()->getValue<bool>("enableCombatMovement");
		// Only when a combat decision (heal, dash, an attack...) is NOT being perfomed, the enemy will orbit around
		if (should_combat_move) {
			float dt_acum = ctx.getNodeVariable<float>(name, DT_ACUM_KEY);
			dt_acum += dt;
			ctx.setNodeVariable(name, DT_ACUM_KEY, dt_acum);

			TCompTransform* h_trans = ctx.getComponent<TCompTransform>();

			// Random direction movement
			if (dt_acum >= period) {
				dt_acum -= period;
				ctx.setNodeVariable(name, DT_ACUM_KEY, dt_acum);

				// Generate random path
				VEC3 microtarget = EngineNavMesh.getRandomPointAroundCircle(h_trans->getPosition(), range);

				ctx.setNodeVariable(name, TARGET_IDX_KEY, 0);

				TNavPath new_path;
				EngineNavMesh.getPath(h_trans->getPosition(), microtarget, new_path);
				ctx.setNodeVariable(name, PATH_KEY, new_path);
			}
			else {
				// Face target
				//CHandle h_face_target = ctx.getBlackboard()->getValue<CHandle>(string_field);
				CHandle h_face_target = getPlayer();
				CEntity* e_face_target = h_face_target;
				VEC3 face_target_pos = e_face_target->getPosition();
				TaskUtils::rotateToFace(h_trans, face_target_pos, rotation_speed, dt);
				//////////////

				// Navigate towards microtarget
				TNavPath path = ctx.getNodeVariable<TNavPath>(name, PATH_KEY);
				if (path.empty()) { return EBTNodeResult::FAILED; }

				navigateToPosition(ctx, move_speed, rotation_speed, 0.2f, 900.f, dt, false);
				///////////////////////////////
			}
		}

		return EBTNodeResult::IN_PROGRESS;
	}
};

class CBTTaskRotateToFace : public IBTTask
{
private:
	const char* DT_ACUM_KEY = "dt_acum";
	const char* DT_ROT_LEFT = "rot_left";
	const char* DT_ROT_RIGHT = "rot_right";
	float max_rot_by_anim_angle = (float)M_2_PI / 2.f;
	float angle_threshold = 5.f * (float)M_2_PI / 180.f;			// Define a threshold to stop rotating to face

	float rotation_speed;
	float duration;

	bool not_animated = false;																// So that it can be used with enemies that do not have rotation animations

public:
	void init() override {
		// Load generic parameters
		rotation_speed = number_field[0];
		duration = number_field[1];
		not_animated = bool_field;
	}

	void onEnter(CBTContext& ctx) override {
		ctx.setNodeVariable(name, DT_ACUM_KEY, 0.f);
		ctx.setNodeVariable(name, DT_ROT_LEFT, false);
		ctx.setNodeVariable(name, DT_ROT_RIGHT, false);
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		
		TCompTransform* h_trans = ctx.getComponent<TCompTransform>();

		// CHandle h_face_target = ctx.getBlackboard()->getValue<CHandle>(string_field);
		CHandle h_face_target = getPlayer();
		CEntity* e_face_target = h_face_target;
		VEC3 face_target_pos = e_face_target->getPosition();

		// Rotate to the left using the animation if the previous rotation was higher than a threshold
		if (ctx.getNodeVariable<bool>(name, DT_ROT_LEFT) && !not_animated) {
			TaskUtils::rotateToFace(h_trans, face_target_pos, rotation_speed, dt);
			return tickCondition(ctx, "is_turning_left", dt);
		}		
		
		// Rotate to the right using the animation if the previous rotation was higher than a threshold
		if (ctx.getNodeVariable<bool>(name, DT_ROT_RIGHT) && !not_animated) {
			TaskUtils::rotateToFace(h_trans, face_target_pos, rotation_speed, dt);
			return tickCondition(ctx, "is_turning_right", dt);
		}

		float dt_acum = ctx.getNodeVariable<float>(name, DT_ACUM_KEY);
		dt_acum += dt;
		ctx.setNodeVariable(name, DT_ACUM_KEY, dt_acum);

		// Check if finished. If duration is 0, it will rotate all the time unless something stops it in the behavior tree 
		// or the angle between the target and the enemy is below a threshold
		if (dt_acum >= duration && duration != 0.f) {
			return EBTNodeResult::SUCCEEDED;
		}
		else {

			// If the rotation is higher than a threshold, rotate left
			if (h_trans->getYawRotationToAimTo(face_target_pos) > max_rot_by_anim_angle && !not_animated) {
				ctx.setNodeVariable(name, DT_ROT_LEFT, true);
				return tickCondition(ctx, "is_turning_left", dt);
			}			
			
			// If the rotation is lower than a threshold, rotate right
			if (h_trans->getYawRotationToAimTo(face_target_pos) < -max_rot_by_anim_angle && !not_animated) {
				ctx.setNodeVariable(name, DT_ROT_RIGHT, true);
				return tickCondition(ctx, "is_turning_right", dt);
			}

			// Face target and check previously if the angle is below the threshold
			float angle_to_target = h_trans->getYawRotationToAimTo(face_target_pos);
			if (abs(angle_to_target) <= angle_threshold)
				return EBTNodeResult::SUCCEEDED;
			
			TaskUtils::rotateToFace(h_trans, face_target_pos, rotation_speed, dt);

			return EBTNodeResult::IN_PROGRESS;
		}
	}
};

class CBTTaskBackstep : public IBTTask
{
private:
	

public:
	void init() override {
		// Load generic parameters
		move_speed = number_field[0];

		// Set animation callbacks
		callbacks.onStartup = [&](CBTContext& ctx, float dt)
		{
			TCompTransform* h_trans = ctx.getComponent<TCompTransform>();
			TCompAIControllerBase* h_controller = ctx.getComponent<TCompAIControllerBase>();

			TaskUtils::moveForward(ctx, -move_speed, dt);
		};
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		return tickCondition(ctx, "is_backstepping", dt);
	}
};

class CBTTaskDash : public IBTTask
{
private:
	
	float rotation_speed;

public:
	void init() override {
		// Load generic parameters
		move_speed = number_field[0];
		rotation_speed = number_field[1];

		// Set animation callbacks
		callbacks.onStartup = [&](CBTContext& ctx, float dt)
		{
			CEntity* player = getPlayer();
			VEC3 player_pos = player->getPosition();

			TCompTransform* h_trans = ctx.getComponent<TCompTransform>();
			TCompAIControllerBase* h_controller = ctx.getComponent<TCompAIControllerBase>();

			TaskUtils::rotateToFace(h_trans, player_pos, rotation_speed, dt);
		};
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		return tickCondition(ctx, "is_dashing", dt);
	}
};

class CBTTaskDodge : public IBTTask
{
private:
	float rotation_speed;

public:
	void init() override {
		// Load generic parameters
		move_speed = number_field[0];
		rotation_speed = number_field[1];

		// Set animation callbacks
		callbacks.onActive = [&](CBTContext& ctx, float dt)
		{
			CEntity* player = getPlayer();

			TCompTransform* h_trans = ctx.getComponent<TCompTransform>();
			TCompAIControllerBase* h_controller = ctx.getComponent<TCompAIControllerBase>();

			// Rotate while dodging
			VEC3 player_pos = player->getPosition();
			TaskUtils::rotateToFace(h_trans, player_pos, rotation_speed, dt);

			// Dodge left or right depending on the rotation multiplier sign
			TaskUtils::moveAnyDir(ctx, h_trans->getRight(), move_speed * ctx.getNodeVariable<float>(name, "rot_multiplier"), dt);
		};
	}

	// Executed on first frame
	void onEnter(CBTContext& ctx) override {
		bool dodge_left = (rand() % 100) <= 50;
		std::string fsm_var = dodge_left ? "is_dodging_left" : "is_dodging_right";
		float dodge_dir_multip = dodge_left ? -1.0f : 1.0f;

		ctx.setNodeVariable(name, "current_fsm_var", fsm_var);
		ctx.setNodeVariable(name, "rot_multiplier", dodge_dir_multip);
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		std::string fsm_var = ctx.getNodeVariable<std::string>(name, "current_fsm_var");
		return tickCondition(ctx, fsm_var, dt);
	}
};

class CBTTaskWarpDodge : public IBTTask
{
private:
	float distance;
	float rotation_speed;

public:
	void init() override {
		// Load generic parameters
		distance = number_field[0];
		rotation_speed = number_field[1];

		// Set animation callbacks
		callbacks.onStartupFinished = [&](CBTContext& ctx, float dt)
		{
			// Get player position and fwd
			CEntity* player = getPlayer();
			TCompTransform* t_player = player->get<TCompTransform>();
			VEC3 player_pos = t_player->getPosition();
			VEC3 player_fwd = t_player->getForward();

			VEC3 target_pos = player_pos - distance * player_fwd;

			// Modify my position
			TCompCollider* h_collider = ctx.getComponent<TCompCollider>();
			h_collider->setFootPosition(target_pos);
		};

		callbacks.onActive = [&](CBTContext& ctx, float dt)
		{
			CEntity* player = getPlayer();
			VEC3 player_pos = player->getPosition();
			TCompTransform* h_trans = ctx.getComponent<TCompTransform>();
			TaskUtils::rotateToFace(h_trans, player_pos, rotation_speed, dt);
		};
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		return tickCondition(ctx, "is_warp_dodging", dt);
	}
};

class CBTTaskLookAround : public IBTTask
{
private:
	const char* ROT_DIR_KEY = "rotating_dir";
	const char* MAX_ARC_KEY = "max_arc_angle";
	const char* IS_ROTATING_KEY = "is_rotating";
	const char* _KEY = "is_rotating";

	float rotation_speed;
	float max_degrees;				// Max degrees to rotate around the center
	float time_between_rot;

public:
	void init() override {
		// Load generic parameters
		rotation_speed = number_field[0];
		max_degrees = number_field[1];
		time_between_rot = number_field[2];
	}

	// Executed on first frame
	void onEnter(CBTContext& ctx) override {
		ctx.setNodeVariable(name, ROT_DIR_KEY, 1.f);

		// it's the first rotation, only rotate half of the max angular distance
		ctx.setNodeVariable(name, MAX_ARC_KEY, max_degrees / 2.f);

		ctx.setNodeVariable(name, IS_ROTATING_KEY, false);

		ctx.setNodeVariable(name, "dt_acum", 0.f);
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {

		float rotation_dir = ctx.getNodeVariable<float>(name, ROT_DIR_KEY);

		// If it is not rotating, change the look at angle
		if (!ctx.getNodeVariable<bool>(name, IS_ROTATING_KEY)) {
			float rot_angle = ctx.getNodeVariable<float>(name, MAX_ARC_KEY) * rotation_dir;
			TaskUtils::changeLookAtAngle(ctx.getOwnerEntity(), rot_angle);
			ctx.setNodeVariable(name, IS_ROTATING_KEY, true);
			return EBTNodeResult::IN_PROGRESS;
		}

		// If it was rotating, check if it has to change the rotation
		TCompSkelLookAt* h_skellookat = ctx.getComponent<TCompSkelLookAt>();
		if(h_skellookat->isLookingAtTarget()){
			
			// Count the time between rotations
			float dt_acum = ctx.getNodeVariable<float>(name, "dt_acum") + dt;
			ctx.setNodeVariable(name, "dt_acum", dt_acum);

			// If the time is up, change the rotation direction
			if (dt_acum > time_between_rot) {
				ctx.setNodeVariable(name, "dt_acum", 0.f);
				ctx.setNodeVariable(name, IS_ROTATING_KEY, false);
				ctx.setNodeVariable(name, ROT_DIR_KEY, rotation_dir * -1.f);
				ctx.setNodeVariable(name, MAX_ARC_KEY, max_degrees);
			}		
		}

		return EBTNodeResult::IN_PROGRESS;
	}
};

class CBTTaskMoveBackwards : public IBTTask
{
private:
	
	float distance;
	float acceptance_dist;

public:
	void init() override {
		move_speed = number_field[0];
		distance = number_field[1];
		acceptance_dist = number_field[2];
	}

	void onEnter(CBTContext& ctx) override {
		ctx.setNodeVariable(name, "curr_distance", distance);
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {

		// Start from the current position to the next position backwards
		TCompTransform* h_trans = ctx.getComponent<TCompTransform>();
		VEC3 origin = h_trans->getPosition();
		VEC3 destination = origin + (-1) * h_trans->getForward() * move_speed * dt;

		// Check if it is possible to move backwards (if there is a path from the origin to the destination
		// Raycast returns true if it has hit a wall in the navigation mesh
		bool has_hit_obstacle = EngineNavMesh.raycast(origin, destination);
		
		// Get the current distance, substracting the maximum distance to the distance to move
		float curr_distance = ctx.getNodeVariable<float>(name, "curr_distance") - VEC3::Distance(origin, destination);

		// Move backwards if it has not reached the required distance and if there is a path
		if (curr_distance > acceptance_dist && !has_hit_obstacle) {
			TCompAIControllerBase* h_controller = ctx.getComponent<TCompAIControllerBase>();
			
			// Move backwards
			TaskUtils::moveForward(ctx, -move_speed, dt);
			
			// Store the current distance
			ctx.setNodeVariable(name, "curr_distance", curr_distance);

			return EBTNodeResult::IN_PROGRESS;
		}
		else {
			return EBTNodeResult::SUCCEEDED;
		}
	}

};

#pragma endregion

#pragma region Melee Enemies

class CBTTaskDashStrike : public IBTTask
{
private:
	
	float rotation_speed;
	int damage;

public:
	void init() override {
		// Load generic parameters
		move_speed = number_field[0];
		rotation_speed = number_field[1];
		damage = (int)getAdjustedParameter(TCompGameManager::EParameterDDA::DAMAGE, number_field[2]);

		// Set animation callbacks
		callbacks.onStartup = [&](CBTContext& ctx, float dt)
		{
			float move_speed_factor = -0.17f;

			CEntity* player = getPlayer();
			VEC3 player_pos = player->getPosition();

			TCompTransform* h_trans = ctx.getComponent<TCompTransform>();
			TCompAIControllerBase* h_controller = ctx.getComponent<TCompAIControllerBase>();

			TaskUtils::rotateToFace(h_trans, player_pos, rotation_speed, dt);
		};

		callbacks.onStartupFinished = [&](CBTContext& ctx, float dt)
		{
			TCompAIControllerBase* controller = ctx.getComponent<TCompAIControllerBase>();
			controller->setWeaponStatusAndDamage(true, damage);
			controller->setWeaponPartStatus(true);

			ctx.setNodeVariable(name, "allow_aborts", false);

			TCompTransform* h_trans = ctx.getComponent<TCompTransform>();
			spawnParticles("data/particles/compute_dash_smoke_particles.json", h_trans->getPosition() + h_trans->getForward() * 0.1f, h_trans->getPosition());
		};

		callbacks.onActiveFinished = [&](CBTContext& ctx, float dt)
		{
			TCompAIControllerBase* controller = ctx.getComponent<TCompAIControllerBase>();
			controller->setWeaponStatusAndDamage(false);

			ctx.setNodeVariable(name, "allow_aborts", true);
		};

		callbacks.onRecoveryFinished = [&](CBTContext& ctx, float dt)
		{
			TCompAIControllerBase* controller = ctx.getComponent<TCompAIControllerBase>();
			controller->setWeaponPartStatus(false);
		};
	}

	// Executed on first frame
	void onEnter(CBTContext& ctx) override {
		ctx.setNodeVariable(name, "allow_aborts", true);
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		return tickCondition(ctx, "is_dash_striking", dt, ctx.getNodeVariable<bool>(name, "allow_aborts"));
	}
};


class CBTTaskThrustAttack : public IBTTask
{
private:
	
	float rotation_speed;
	float range;
	int damage;
	int combo_prob = 20;

	int num_regular_hits = 2;						// It will have only two regular hits
	int max_combo_hits = 3;							// It will perform maximum 3 hits

public:
	void init() override {
		// Load generic parameters
		move_speed = number_field[0];
		rotation_speed = number_field[1];
		combo_prob = (int)number_field[2];
		damage = (int)getAdjustedParameter(TCompGameManager::EParameterDDA::DAMAGE, number_field[3]);

		// Set animation callbacks
		callbacks.onStartup = [&](CBTContext& ctx, float dt)
		{
			float move_speed_factor = 0.5f;

			CEntity* player = getPlayer();
			VEC3 player_pos = player->getPosition();

			TCompTransform* h_trans = ctx.getComponent<TCompTransform>();
			TCompAIControllerBase* h_controller = ctx.getComponent<TCompAIControllerBase>();

			TaskUtils::rotateToFace(h_trans, player_pos, rotation_speed, dt);
			TaskUtils::moveForward(ctx, move_speed * move_speed_factor, dt);
		};

		callbacks.onStartupFinished = [&](CBTContext& ctx, float dt)
		{
			// Pause the attack. At the end enable the damage of the weapon and disable aborts
			TaskUtils::pauseAction(ctx, name, [&](CBTContext& ctx) {
				TCompAIControllerBase* controller = ctx.getComponent<TCompAIControllerBase>();
				controller->setWeaponStatusAndDamage(true, damage);
				controller->setWeaponPartStatus(true);
				ctx.setNodeVariable(name, "allow_aborts", false);
			});
		};

		callbacks.onActive = [&](CBTContext& ctx, float dt)
		{
			float move_speed_factor = 1.75f;

			CEntity* player = getPlayer();
			VEC3 player_pos = player->getPosition();

			TCompTransform* h_trans = ctx.getComponent<TCompTransform>();
			TCompAIControllerBase* h_controller = ctx.getComponent<TCompAIControllerBase>();

			//TaskUtils::rotateToFace(h_trans, player_pos, rotation_speed, dt);
			TaskUtils::moveForward(ctx, move_speed * move_speed_factor, dt);
		};

		callbacks.onActiveFinished = [&](CBTContext& ctx, float dt)
		{		
			// Check if it can perform another hit
			int current_hits = ctx.getNodeVariable<int>(name, "current_hits");
			bool continue_combo = (current_hits < max_combo_hits) && (rand() % 100 < combo_prob);
					
			// If the combo continues, cancel the current animation and get the next attack to be executed
			if (continue_combo) {
			
				ctx.setFSMVariable("combo_attack_active", true);
				ctx.setNodeVariable(name, "current_hits", ++current_hits);						// Always increment the amount of hits struck
					
				int next_hit = 1 + std::get<int>(ctx.getFSMVariable("is_thrust_attacking")) % num_regular_hits;
				ctx.setFSMVariable("is_thrust_attacking", next_hit);
			}
			else {
				// If the combo does not continue, inactive the combo so that the variable can be reset in the FSM
				ctx.setFSMVariable("combo_attack_active", false);
			}

			TCompAIControllerBase* controller = ctx.getComponent<TCompAIControllerBase>();
			controller->setWeaponStatusAndDamage(false);
			controller->setWeaponPartStatus(false);
			ctx.setNodeVariable(name, "allow_aborts", true);
		};
	}

	// Executed on first frame
	void onEnter(CBTContext& ctx) override {
		ctx.setNodeVariable(name, "allow_aborts", true);
		ctx.setNodeVariable(name, "pause_animation", false);
		ctx.setNodeVariable(name, "resume_time", Random::range(0.4f, 0.8f));

		// Initialize the amount of hits struck
		ctx.setNodeVariable(name, "current_hits", 1);
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		TaskUtils::resumeActionAt(ctx, name, ctx.getNodeVariable<float>(name, "resume_time"), dt);
		EBTNodeResult result = tickCondition(ctx, "is_thrust_attacking", dt, ctx.getNodeVariable<bool>(name, "allow_aborts"));

		// If the task has finished, set the variables to -1 for another execution
		if (result == EBTNodeResult::SUCCEEDED)
			cleanTask(ctx);

		return result;
	}

	void cleanTask(CBTContext& ctx) override {
		ctx.setFSMVariable("is_thrust_attacking", -1);
	}
};

class CBTTaskCircularAttack : public IBTTask
{
private:
	float rotation_speed;
	float range;
	int damage;

public:
	void init() override {
		// Load generic parameters
		move_speed = number_field[0];
		rotation_speed = number_field[1];
		range = number_field[2];
		damage = (int)getAdjustedParameter(TCompGameManager::EParameterDDA::DAMAGE, number_field[3]);

		// Set animation callbacks
		callbacks.onStartup = [&](CBTContext& ctx, float dt)
		{
			float move_speed_factor = 0.6f;

			CEntity* player = getPlayer();
			VEC3 player_pos = player->getPosition();

			TCompTransform* h_trans = ctx.getComponent<TCompTransform>();
			TCompAIControllerBase* h_controller = ctx.getComponent<TCompAIControllerBase>();

			TaskUtils::rotateToFace(h_trans, player_pos, rotation_speed, dt);
			TaskUtils::moveForward(ctx, move_speed * move_speed_factor, dt);
		};

		callbacks.onStartupFinished = [&](CBTContext& ctx, float dt)
		{
			TCompAIControllerBase* controller = ctx.getComponent<TCompAIControllerBase>();
			controller->setWeaponStatusAndDamage(true, damage);
			controller->setWeaponPartStatus(true);

			ctx.setNodeVariable(name, "allow_aborts", false);
		};

		callbacks.onActiveFinished = [&](CBTContext& ctx, float dt)
		{
			TCompAIControllerBase* controller = ctx.getComponent<TCompAIControllerBase>();
			controller->setWeaponStatusAndDamage(false);
			controller->setWeaponPartStatus(false);

			ctx.setNodeVariable(name, "allow_aborts", true);
		};

		callbacks.onRecovery = [&](CBTContext& ctx, float dt)
		{
			CEntity* player = getPlayer();
			VEC3 player_pos = player->getPosition();

			TCompTransform* h_trans = ctx.getComponent<TCompTransform>();
			TCompAIControllerBase* h_controller = ctx.getComponent<TCompAIControllerBase>();

			TaskUtils::rotateToFace(h_trans, player_pos, rotation_speed, dt);
		};
	}

	// Executed on first frame
	void onEnter(CBTContext& ctx) override {
		ctx.setNodeVariable(name, "allow_aborts", true);
		ctx.setNodeVariable(name, "resume_time", Random::range(0.5f, 0.8f));
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		TaskUtils::resumeActionAt(ctx, name, ctx.getNodeVariable<float>(name, "resume_time"), dt);
		return tickCondition(ctx, "is_circular_attacking", dt, ctx.getNodeVariable<bool>(name, "allow_aborts"));
	}
};

#pragma endregion

#pragma region Ranged Enemies
// Specific tasks for Ranged Enemies

class CBTTaskRangedAttack : public IBTTask
{
private:
	float rotation_speed;
	int ranged_attack_dmg;
	bool is_homing = false;

public:
	void init() override {
		// Load generic parameters
		rotation_speed = number_field[0];
		ranged_attack_dmg = (int)getAdjustedParameter(TCompGameManager::EParameterDDA::DAMAGE, number_field[1]);
		is_homing = bool_field;

		// Set animation callbacks
		callbacks.onStartup = [&](CBTContext& ctx, float dt)
		{
			CEntity* player = getPlayer();
			VEC3 player_pos = player->getPosition();
			TCompTransform* h_trans = ctx.getComponent<TCompTransform>();
			
			TaskUtils::rotateToFace(h_trans, player_pos, rotation_speed, dt);
		};

		callbacks.onStartupFinished = [&](CBTContext& ctx, float dt)
		{
			TCompTransform* h_trans = ctx.getComponent<TCompTransform>();
			spawnParticles("data/particles/compute_projectile_portal_particles.json", h_trans->getPosition() + h_trans->getForward() * 0.4f, h_trans->getPosition());
		};

		callbacks.onActiveFinished = [&](CBTContext& ctx, float dt)
		{
			TCompTransform* h_trans = ctx.getComponent<TCompTransform>();
			CEntity* player = getPlayer();						

			// Shoot origin by default
			VEC3 shoot_orig = h_trans->getPosition();
			VEC3 shoot_dest = player->getPosition();

			// Move to specific position
			TCompParent* parent = ctx.getComponent<TCompParent>();
			if (parent) {
				CEntity* child = parent->getChildByName("SpawnPoint");
				if (child) {
					shoot_orig = child->getPosition();
				}
			}
			
			TaskUtils::rotateToFace(h_trans, shoot_dest, rotation_speed, dt);
			TaskUtils::spawnProjectile(shoot_orig, shoot_dest, ranged_attack_dmg, false, is_homing);

			ctx.setNodeVariable(name, "allow_aborts", true);
		};
	}

	// Executed on first frame
	void onEnter(CBTContext& ctx) override {
		CEntity* player = getPlayer();
		ctx.setNodeVariable(name, "last_player_pos", player->getPosition());
		ctx.setNodeVariable(name, "allow_aborts", false);

		// To count the time to stop the animation
		ctx.setNodeVariable(name, "dt_acum", 0.f);
		ctx.setNodeVariable(name, "pause_animation", false);
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		
		return tickCondition(ctx, "is_ranged_attacking", dt, ctx.getNodeVariable<bool>(name, "allow_aborts"));
	}
};

#pragma endregion

#pragma region Gard

class CBTTaskGardAreaAttack : public IBTTask
{
private:
	int area_damage;
	float area_radius;
	float rotation_speed = 50.f;

public:
	void init() override {
		// Load generic parameters
		area_damage = (int)getAdjustedParameter(TCompGameManager::EParameterDDA::DAMAGE, number_field[0]);
		area_radius = number_field[1];

		callbacks.onFirstFrame = [&](CBTContext& ctx, float dt)
		{
			TaskUtils::pauseAction(ctx, name);
		};

		// Set animation callbacks
		callbacks.onStartup = [&](CBTContext& ctx, float dt)
		{
			CEntity* player = getPlayer();
			VEC3 player_pos = player->getPosition();

			TCompTransform* h_trans = ctx.getComponent<TCompTransform>();
			TCompAIControllerBase* h_controller = ctx.getComponent<TCompAIControllerBase>();

			TaskUtils::rotateToFace(h_trans, player_pos, rotation_speed, dt);
		};

		// Set animation callbacks
		/*callbacks.onActive = [&](CBTContext& ctx, float dt)
		{
			CEntity* player = getPlayer();
			VEC3 player_pos = player->getPosition();

			TCompTransform* h_trans = ctx.getComponent<TCompTransform>();
			TCompAIControllerBase* h_controller = ctx.getComponent<TCompAIControllerBase>();

			TaskUtils::rotateToFace(h_trans, player_pos, rotation_speed, dt);
		};*/

		callbacks.onActiveFinished = [&](CBTContext& ctx, float dt)
		{
			CEntity* outputCamera = getEntityByName("camera_mixed");
			TCompCameraShake* shaker = outputCamera->get<TCompCameraShake>();
			shaker->shakeOnce(4.f, 0.02f, 0.3f);

			CEntity* e_gard = ctx.getOwnerEntity();
			if (!e_gard)
				return;
			TCompTransform* h_trans_gard = e_gard->get<TCompTransform>();
			
			VEC3 handPos = TaskUtils::getBoneWorldPosition(e_gard, "Bip001 R Finger01");
			TaskUtils::castAreaAttack(e_gard, handPos, area_radius, area_damage);
		};
	}

	void onEnter(CBTContext& ctx) override {
		ctx.setNodeVariable(name, "resume_time", Random::range(0.5f, 1.5f));
		ctx.setNodeVariable(name, "pause_animation", false);
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		TaskUtils::resumeActionAt(ctx, name, ctx.getNodeVariable<float>(name, "resume_time"), dt);
		return tickCondition(ctx, "is_area_attacking", dt, false);
	}
};

class CBTTaskGardRangedAttack : public IBTTask
{
private:
	int damage;
	float spawn_freq = 0.2f;							// Frequency in which branches are spawned	
	bool is_random = true;								// Defines if Gard spawns branches in random places or directed to Eon

	void spawnRandomBranches(CBTContext& ctx, float dt) {

		float curr_time = ctx.getNodeVariable<float>(name, "dt_acum");

		curr_time += dt;

		if (curr_time >= spawn_freq) {

			CEntity* e_gard = ctx.getOwnerEntity();
			CEntity* e_player = getPlayer();
			if (!e_gard || !e_player)
				return;
			TCompTransform* c_player_trans = e_player->get<TCompTransform>();
			VEC3 pos = TaskUtils::getRandomPosAroundPoint(c_player_trans->getPosition(), 4.5f);
				
			VEC3 gardPos = e_gard->getPosition();
			if ((pos - gardPos).Length() < 1.5f)
				return;

			TaskUtils::spawnBranch(pos, damage, 7.5f);

			// initialize dt_acum
			ctx.setNodeVariable(name, "dt_acum", 0.f);
		}
		else
		{
			ctx.setNodeVariable(name, "dt_acum", curr_time);
		}

	}

	void spawnDirectedBranches(CBTContext& ctx, float dt) {
		float curr_time = ctx.getNodeVariable<float>(name, "dt_acum");
			
		curr_time += dt;
		if (!ctx.isNodeVariableValid(name, "branch_pos")) return;
		VEC3 branch_pos = ctx.getNodeVariable<VEC3>(name, "branch_pos");

		if (curr_time >= spawn_freq) {

			CEntity* e_player = getPlayer();
			if (!e_player)
				return;
			TCompTransform* c_player_trans = e_player->get<TCompTransform>();
			VEC3 next_pos = c_player_trans->getPosition() - branch_pos;
			next_pos.Normalize();

			branch_pos = branch_pos + next_pos * 2.f;
			TaskUtils::spawnBranch(branch_pos, damage, 7.5f);

			ctx.setNodeVariable(name, "branch_pos", branch_pos);
			// initialize dt_acum
			ctx.setNodeVariable(name, "dt_acum", 0.f);
			return;
		}
		ctx.setNodeVariable(name, "dt_acum", curr_time);
	}

public:
	void init() override {
		// Load generic parameters
		damage = (int)getAdjustedParameter(TCompGameManager::EParameterDDA::DAMAGE, number_field[0]);
		spawn_freq = number_field[1];
		is_random = bool_field;	

		callbacks.onActive = [&](CBTContext& ctx, float dt)
		{
			if (is_random) {
				spawnRandomBranches(ctx, dt);
			}
			else {
				spawnDirectedBranches(ctx, dt);
			}
		};
	}

	void onEnter(CBTContext& ctx) override {
		ctx.setNodeVariable(name, "dt_acum", 0.f);

		// If it is directed, start where Gard is located
		if (!is_random) {
			TCompTransform* c_gard_trans = ctx.getComponent<TCompTransform>();
			if (c_gard_trans)	ctx.setNodeVariable(name, "branch_pos", c_gard_trans->getPosition());
		}
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		return tickCondition(ctx, "is_ranged_attacking", dt, false);
	}
};

class CBTTaskGardSweepAttack : public IBTTask
{
private:
	int damage;
	float rotation_speed = 50.f;

public:
	void init() override {
		// Load generic parameters
		damage = (int)getAdjustedParameter(TCompGameManager::EParameterDDA::DAMAGE, number_field[0]);

		// Set animation callbacks
		callbacks.onStartup = [&](CBTContext& ctx, float dt)
		{
			CEntity* player = getPlayer();
			VEC3 player_pos = player->getPosition();

			TCompTransform* h_trans = ctx.getComponent<TCompTransform>();
			TCompAIControllerBase* h_controller = ctx.getComponent<TCompAIControllerBase>();

			TaskUtils::rotateToFace(h_trans, player_pos, rotation_speed, dt);
		};

		callbacks.onStartupFinished = [&](CBTContext& ctx, float dt)
		{
			TaskUtils::pauseAction(ctx, name, [&](CBTContext& ctx) {
				TCompAIControllerBase* controller = ctx.getComponent<TCompAIControllerBase>();
				controller->setWeaponStatusAndDamage(true, damage);
			});
		};

		// Set animation callbacks
		/*callbacks.onActive = [&](CBTContext& ctx, float dt)
		{
			CEntity* player = getPlayer();
			VEC3 player_pos = player->getPosition();

			TCompTransform* h_trans = ctx.getComponent<TCompTransform>();
			TCompAIControllerBase* h_controller = ctx.getComponent<TCompAIControllerBase>();

			TaskUtils::rotateToFace(h_trans, player_pos, rotation_speed, dt);
		};*/

		callbacks.onActiveFinished = [&](CBTContext& ctx, float dt)
		{
			 TCompAIControllerBase* controller = ctx.getComponent<TCompAIControllerBase>();
			 controller->setWeaponStatusAndDamage(false);
		};
	}

	void onEnter(CBTContext& ctx) override {
		ctx.setNodeVariable(name, "resume_time", Random::range(0.5f, 2.f));
		ctx.setNodeVariable(name, "pause_animation", false);
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		TaskUtils::resumeActionAt(ctx, name, ctx.getNodeVariable<float>(name, "resume_time"), dt);
		return tickCondition(ctx, "is_sweep_attacking", dt, false);
	}
};

class CBTTaskGardJumpAttack : public IBTTask
{
private:
	int area_damage;
	float area_radius;

public:
	void init() override {
		// Load generic parameters
		area_damage = (int)getAdjustedParameter(TCompGameManager::EParameterDDA::DAMAGE, number_field[0]);
		area_radius = number_field[1];

		callbacks.onFirstFrame = [&](CBTContext& ctx, float dt)
		{
			TaskUtils::pauseAction(ctx, name);
		};

		callbacks.onStartup = [&](CBTContext& ctx, float dt)
		{
			TCompTransform* c_gard_trans = ctx.getComponent<TCompTransform>();
			CEntity* e_player = getPlayer();
			TaskUtils::rotateToFace(c_gard_trans, e_player->getPosition(), 30, dt);
		};

		callbacks.onActiveFinished = [&](CBTContext& ctx, float dt)
		{
			CEntity* outputCamera = getEntityByName("camera_mixed");
			TCompCameraShake* shaker = outputCamera->get<TCompCameraShake>();
			shaker->shakeOnce(9.f, 0.02f, 0.3f);

			CEntity* e_gard = ctx.getOwnerEntity();
			if (!e_gard)
				return;
			TCompTransform* h_trans_gard = e_gard->get<TCompTransform>();

			// Alex: Casting 2 attacks, 1 per arm, but I reduce the damage by 2
			// so we have 2 particles and the same damage
			VEC3 f0 = TaskUtils::getBoneWorldPosition(e_gard, "Bip001 R Finger01");
			VEC3 f1 = TaskUtils::getBoneWorldPosition(e_gard, "Bip001 L Finger01");
			TaskUtils::castAreaAttack(e_gard, f0, area_radius * 0.5f, (int)(area_damage * 0.5f));
			TaskUtils::castAreaAttack(e_gard, f1, area_radius * 0.5f, (int)(area_damage * 0.5f));
		};
	}

	void onEnter(CBTContext& ctx) override {
		ctx.setNodeVariable(name, "resume_time", Random::range(0.5f, 1.5f));
		ctx.setNodeVariable(name, "pause_animation", false);
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		TaskUtils::resumeActionAt(ctx, name, ctx.getNodeVariable<float>(name, "resume_time"), dt);
		return tickCondition(ctx, "is_jump_attacking", dt, false);
	}
};

class CBTTaskGardAreaDelay : public IBTTask
{
private:
	float duration;
	float max_radius;

public:
	void init() override {
		
		// Load generic parameters
		duration = number_field[0];
		max_radius = number_field[1];

		// Set animation callbacks
		callbacks.onActiveFinished = [&](CBTContext& ctx, float dt)
		{
			CEntity* e_gard = ctx.getOwnerEntity();
			if (!e_gard)
				return;

			// Cast an area delay without target, generated where Gard is
			// TaskUtils::castAreaDelay(e_gard, duration, CHandle());
			TaskUtils::castWaveDelay(e_gard, 1.f, max_radius, duration);
		};
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		return tickCondition(ctx, "is_casting_area_delay", dt, false);
	}
};

class CBTTaskGardDeath : public IBTTask
{
public:
	void init() override {

		// Set animation callbacks
		callbacks.onFirstFrame = [&](CBTContext& ctx, float dt)
		{
			// Destroy branches in the scene
			getObjectManager<TCompGardBranch>()->forEach(
				[&](TCompGardBranch* c_branch) {
					c_branch->destroy();
				}
			);

			// Stop any camera shake if there is one
			CEntity* outputCamera = getEntityByName("camera_mixed");
			TCompCameraShake* shaker = outputCamera->get<TCompCameraShake>();
			shaker->stop(0.1f);

			// Cinematics
			EngineLua.executeScript("CinematicGardDeath()");
		};
	}

	void onEnter(CBTContext& ctx) override {
		TaskUtils::resumeAction(ctx, name);
		TaskUtils::dissolveAt(ctx, 30.f, 3.5f);
		ctx.setIsDying(true);
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		return tickCondition(ctx, "is_dead", dt);
	}
};

#pragma endregion

#pragma region Cygnus Tasks

class CBTTaskCygnusAreaDelay : public IBTTask
{
private:
	float duration;
	float max_radius;

public:
	void init() override {

		// Load generic parameters
		duration = number_field[0];
		max_radius = number_field[1];

		// Set animation callbacks
		callbacks.onActiveFinished = [&](CBTContext& ctx, float dt)
		{
			CEntity* e_cygnus = ctx.getOwnerEntity();
			if (!e_cygnus)
				return;

			// Cast an area delay without target, generated where Cygnus is
			TaskUtils::castWaveDelay(e_cygnus, 1.f, max_radius, duration);
		};
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		return tickCondition(ctx, "is_casting_area_delay", dt, false);
	}
};

class CBTTaskCygnusRangedAttack : public IBTTask
{
private:
	float rotation_speed;
	int damage;
	int projectiles_num;
	float shots_period;

public:
	void init() override {
		// Load generic parameters
		rotation_speed = number_field[0];
		damage = (int)getAdjustedParameter(TCompGameManager::EParameterDDA::DAMAGE, number_field[1]);
		projectiles_num = (int)number_field[2];
		shots_period = number_field[3];
		move_speed = 3.f;

		// Set animation callbacks
		callbacks.onStartup = [&](CBTContext& ctx, float dt)
		{
			CEntity* player = getPlayer();
			VEC3 player_pos = player->getPosition();
			TCompTransform* h_trans = ctx.getComponent<TCompTransform>();
			TaskUtils::rotateToFace(h_trans, player_pos, rotation_speed, dt);

			TCompParent* parent = ctx.getComponent<TCompParent>();
			CEntity* hole = parent->getChildByName("Cygnus_black_hole");
			TCompAttachedToBone* socket = hole->get<TCompAttachedToBone>();
			CTransform& t = socket->getLocalTransform();
			t.setScale(damp<VEC3>(t.getScale(), VEC3(0.25f), 8.f, dt));
		};

		callbacks.onActive = [&](CBTContext& ctx, float dt)
		{
			CEntity* player = getPlayer();
			VEC3 player_pos = player->getPosition();
			TCompTransform* h_trans = ctx.getComponent<TCompTransform>();

			float dt_acum = ctx.getNodeVariable<float>(name, "dt_acum");
			
			// Alternative A - Shoot projectiles
			// When time reaches 0, shoot projectiles
			if (dt_acum == 0) {
				VEC3 shoot_orig = h_trans->getPosition() + VEC3(0, 1.f, 0);
				VEC3 shoot_dest = player->getPosition();

				TaskUtils::spawnCygnusProjectiles(shoot_orig, shoot_dest, damage, projectiles_num);

				ctx.setNodeVariable(name, "dt_acum", dt_acum + dt);
			}
			else if (dt_acum <= shots_period) {
				ctx.setNodeVariable(name, "dt_acum", dt_acum + dt);
			}
			else {
				ctx.setNodeVariable(name, "dt_acum", 0.f);
			}

			// Move towards Eon		
			TaskUtils::rotateToFace(h_trans, player_pos, rotation_speed, dt);
			TaskUtils::moveForward(ctx, move_speed, dt);
		};

		callbacks.onRecovery = [&](CBTContext& ctx, float dt)
		{
			TCompParent* parent = ctx.getComponent<TCompParent>();
			CEntity* hole = parent->getChildByName("Cygnus_black_hole");
			TCompAttachedToBone* socket = hole->get<TCompAttachedToBone>();
			CTransform& t = socket->getLocalTransform();
			t.setScale(damp<VEC3>(t.getScale(), VEC3(0.16f), 12.f, dt));
		};

		callbacks.onActiveFinished = [&](CBTContext& ctx, float dt)
		{
			ctx.setNodeVariable(name, "allow_aborts", true);

			// Stop all forces
			CEntity* player = getPlayer();
			TMsgRemoveForces msgForce;
			msgForce.byPlayer = false;
			msgForce.force_origin = "Cygnus";
			player->sendMsg(msgForce);
		};
	}

	// Executed on first frame
	void onEnter(CBTContext& ctx) override {
		ctx.setNodeVariable(name, "allow_aborts", false);

		// To count the time between shots
		ctx.setNodeVariable(name, "dt_acum", 0.f);
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {

		return tickCondition(ctx, "is_ranged_attacking", dt, ctx.getNodeVariable<bool>(name, "allow_aborts"));
	}
};

class CBTTaskCygnusMeleeAttack : public IBTTask
{
private:

	float rotation_speed = 50.f;
	int damage = 20;
	int combo_prob = 20;
	int strong_prob = 30;
	int strong_dmg_multiplier = 2;
	bool is_phase_2 = false;

	int num_regular_hits = 2;						// For phase 1 it will have only two regular hits. In Phase 2, there are 3 regular hits
	int max_combo_hits = 3;							// For phase 1 it will perform maximum 3 hits. In Phase 2, it can perform up to 4 hits

public:
	void init() override {
		// Load generic parameters
		damage = (int)getAdjustedParameter(TCompGameManager::EParameterDDA::DAMAGE, number_field[0]);
		combo_prob = (int)number_field[1];
		strong_prob = (int)number_field[2];
		rotation_speed = number_field[3];
		is_phase_2 = bool_field;

		num_regular_hits = is_phase_2 ? 3 : 2;
		max_combo_hits = is_phase_2 ? 4 : 3;

		// Set animation callbacks
		callbacks.onStartup = [&](CBTContext& ctx, float dt)
		{
			CEntity* player = getPlayer();
			VEC3 player_pos = player->getPosition();

			TCompTransform* h_trans = ctx.getComponent<TCompTransform>();
			TCompAIControllerBase* h_controller = ctx.getComponent<TCompAIControllerBase>();

			TaskUtils::rotateToFace(h_trans, player_pos, rotation_speed, dt);
		};

		callbacks.onStartupFinished = [&](CBTContext& ctx, float dt)
		{
			TCompAIControllerBase* controller = ctx.getComponent<TCompAIControllerBase>();

			// If the current attack is strong add a multiplier to the nominal damage
			int curr_damage = ctx.getNodeVariable<std::string>(name, "current_fsm_var") == "is_strong_attacking" ?
				damage * strong_dmg_multiplier :
				damage;

			controller->setWeaponStatusAndDamage(true, curr_damage);
			ctx.setNodeVariable(name, "allow_aborts", false);
		};

		callbacks.onActive = [&](CBTContext& ctx, float dt)
		{
			CEntity* player = getPlayer();
			VEC3 player_pos = player->getPosition();

			TCompTransform* h_trans = ctx.getComponent<TCompTransform>();
			TCompAIControllerBase* h_controller = ctx.getComponent<TCompAIControllerBase>();

			TaskUtils::rotateToFace(h_trans, player_pos, rotation_speed, dt);
		};

		callbacks.onActiveFinished = [&](CBTContext& ctx, float dt)
		{
			TCompAIControllerBase* controller = ctx.getComponent<TCompAIControllerBase>();

			// Check if it can perform another hit
			int current_hits = ctx.getNodeVariable<int>(name, "current_hits");
			bool continue_combo = (current_hits < max_combo_hits) && (rand() % 100 < combo_prob);

			std::string fsm_var = ctx.getNodeVariable<std::string>(name, "current_fsm_var");
			
			// If the combo continues, cancel the current animation and get the next attack to be executed
			if (continue_combo) {
				
				ctx.setFSMVariable("combo_attack_active", true);
				ctx.setNodeVariable(name, "current_hits", ++current_hits);						// Always increment the amount of hits struck
				
				// If the previous attack was a regular attack, go to the next regular attack or to a strong attack
				if (fsm_var != "is_strong_attacking") {

					// Check if the next attack should be strong
					bool is_strong = rand() % 100 < strong_prob;
					if (!is_strong) {
						int next_hit = 1 + std::get<int>(ctx.getFSMVariable(fsm_var)) % num_regular_hits;	// Mod 2 for phase 1, and Mod 3 for phase 2
						ctx.setFSMVariable(fsm_var, next_hit);
					}
					else {
						ctx.setFSMVariable(fsm_var, 0);
						ctx.setNodeVariable(name, "current_fsm_var", std::string("is_strong_attacking"));
						ctx.setFSMVariable("is_strong_attacking", 1);
					}
										
				}
				else {
					// If the previous attack was strong it should only go to a regular attack
					ctx.setNodeVariable(name, "current_fsm_var", std::string("is_regular_attacking"));
					ctx.setFSMVariable("is_regular_attacking", 1);
					ctx.setFSMVariable("is_strong_attacking", 0);
				}
				
			}
			else {
				// If the combo does not continue, inactive the combo so that the variable can be reset in the FSM
				ctx.setFSMVariable("combo_attack_active", false);
			}

			controller->setWeaponStatusAndDamage(false);
			ctx.setNodeVariable(name, "allow_aborts", true);
		};

		callbacks.onRecoveryFinished = [&](CBTContext& ctx, float dt)
		{
			if (is_phase_2) {
				CEntity* owner = ctx.getOwnerEntity();
				TCompEmissiveMod* mod = owner->get<TCompEmissiveMod>();
				if (mod)
					mod->blendOut();
			}
		};
	}

	// Executed on first frame
	void onEnter(CBTContext& ctx) override {
		ctx.setNodeVariable(name, "allow_aborts", true);

		// Decide if it starts with a strong or regular attack
		bool is_strong = rand() % 100 < strong_prob;

		// Store the current variable from the FSM that is activated
		std::string fsm_var = is_strong ? "is_strong_attacking" : "is_regular_attacking";
		ctx.setNodeVariable(name, "current_fsm_var", fsm_var);

		// Initialize the amount of hits struck
		ctx.setNodeVariable(name, "current_hits", 1);

		if (is_phase_2) {
			CEntity* owner = ctx.getOwnerEntity();
			TCompEmissiveMod* mod = owner->get<TCompEmissiveMod>();
			if(mod)
				mod->blendIn();

			TCompTransform* h_trans = ctx.getComponent<TCompTransform>();
			spawnParticles("data/particles/compute_cygnus_spread_particles.json", h_trans->getPosition() + h_trans->getRight() * 0.1f, h_trans->getPosition(), 2.f);
		}
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		// Only update the variable of the animation that is currently being executed
		std::string fsm_var = ctx.getNodeVariable<std::string>(name, "current_fsm_var");
		EBTNodeResult result = tickCondition(ctx, fsm_var, dt, ctx.getNodeVariable<bool>(name, "allow_aborts"));
		
		// If the task has finished, set the variables to -1 for another execution
		if (result == EBTNodeResult::SUCCEEDED)
			cleanTask(ctx);

		return result;
	}

	void cleanTask(CBTContext& ctx) override {
		ctx.setFSMVariable("is_regular_attacking", -1);
		ctx.setFSMVariable("is_strong_attacking", -1);
	}
};

class CBTTaskCygnusDodge : public IBTTask
{
private:
	float rotation_speed;

public:
	void init() override {
		// Load generic parameters
		move_speed = number_field[0];
		rotation_speed = number_field[1];

		callbacks.onStartupFinished = [&](CBTContext& ctx, float dt)
		{
			ctx.setNodeVariable(name, "allow_aborts", false);
		};

		// Set animation callbacks
		callbacks.onActive = [&](CBTContext& ctx, float dt)
		{
			CEntity* player = getPlayer();

			TCompTransform* h_trans = ctx.getComponent<TCompTransform>();
			TCompAIControllerBase* h_controller = ctx.getComponent<TCompAIControllerBase>();

			// Rotate while dodging
			VEC3 player_pos = player->getPosition();
			TaskUtils::rotateToFace(h_trans, player_pos, rotation_speed, dt);
			
			// Dodge left or right depending on the rotation multiplier sign
			TaskUtils::moveAnyDir(ctx, h_trans->getRight(), move_speed * ctx.getNodeVariable<float>(name, "rot_multiplier"), dt);
		};

		callbacks.onActiveFinished = [&](CBTContext& ctx, float dt)
		{
			ctx.setNodeVariable(name, "allow_aborts", true);
		};
	}

	// Executed on first frame
	void onEnter(CBTContext& ctx) override {
		bool dodge_left = (rand() % 100) <= 50;
		std::string fsm_var = dodge_left ? "is_dodging_left" : "is_dodging_right";
		float dodge_dir_multip = dodge_left ? 1.0f : -1.0f;
		
		ctx.setNodeVariable(name, "current_fsm_var", fsm_var);
		ctx.setNodeVariable(name, "rot_multiplier", dodge_dir_multip);
		ctx.setNodeVariable(name, "allow_aborts", true);
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		std::string fsm_var = ctx.getNodeVariable<std::string>(name, "current_fsm_var");
		return tickCondition(ctx, fsm_var, dt, ctx.getNodeVariable<bool>(name, "allow_aborts"));
	}
};

class CBTTaskCygnusWalkAggressive : public IBTTask
{
private:
	float rotation_speed = 50.f;
	float move_speed;
	int damage = 20;
	int attack_reps = 3;

	float acceptance_dist = 1.f;

public:

	void init() override {
		// Load generic parameters
		move_speed = number_field[0];
		rotation_speed = number_field[1];
		damage = (int)number_field[2];
	}


	EBTNodeResult executeTask(CBTContext& ctx, float dt) {

		TCompTransform* h_trans = ctx.getComponent<TCompTransform>();	
		VEC3 target = ctx.getNodeVariable<VEC3>(name, "current_target");
		float cur_dist = VEC3::Distance(h_trans->getPosition(), target);

		// If it has not reached the target, move towards it
		if (cur_dist > acceptance_dist) {

			TNavPath path;
			EngineNavMesh.getPath(h_trans->getPosition(), target, path);

			// No possible path
			if (path.empty() || path.size() == 1) { return EBTNodeResult::FAILED; }

			// Movement has not reached target

			TCompAIControllerBase* h_controller = ctx.getComponent<TCompAIControllerBase>();
			// Rotate to face target
			TaskUtils::rotateToFace(h_trans, path[1], rotation_speed, dt);

			// Advance forward
			TaskUtils::moveForward(ctx, move_speed, dt);

			return EBTNodeResult::IN_PROGRESS;
		}

		// If it has reached the target and there are more attack reps to go, get the next target
		int curr_rep = ctx.getNodeVariable<int>(name, "current_rep");

		if (curr_rep < attack_reps) {
			// Store the new target
			CHandle h_target = getPlayer();
			CEntity* e_target = h_target;
			VEC3 target = EngineNavMesh.getRandomPointAroundCircle(e_target->getPosition(), 1.f);

			ctx.setNodeVariable(name, "current_target", target);

			// Increase the number of repetitions
			ctx.setNodeVariable(name, "current_rep", ++curr_rep);

			return EBTNodeResult::IN_PROGRESS;
		}
			
		cleanTask(ctx);
		return EBTNodeResult::SUCCEEDED;
	}

	void onEnter(CBTContext& ctx) override {
		TCompAIControllerBase* controller = ctx.getComponent<TCompAIControllerBase>();
		controller->setWeaponStatusAndDamage(true, damage);

		// Store current target as the player's position
		CHandle h_target = getPlayer();
		CEntity* e_target = h_target;
		VEC3 target = EngineNavMesh.getRandomPointAroundCircle(e_target->getPosition(), 1.f);

		ctx.setNodeVariable(name, "current_target", target);
		ctx.setNodeVariable(name, "current_rep", 1);
		ctx.setFSMVariable("is_walking_aggressive", 1);
	}

	void cleanTask(CBTContext& ctx) override {
		// Set the damage of the weapon to false
		TCompAIControllerBase* controller = ctx.getComponent<TCompAIControllerBase>();
		controller->setWeaponStatusAndDamage(false);

		// Finish the animation
		ctx.setFSMVariable("is_walking_aggressive", 0);									
	}

};

class CBTTaskCygnusDeath : public IBTTask
{
public:
	void init() override {}

	void onEnter(CBTContext& ctx) override {
#if PLAY_CINEMATICS
		TaskUtils::resumeAction(ctx, name);
		ctx.setIsDying(true);

		// Add a fade out to start the animation
		EngineUI.fadeOut(0.7f, 0.2f, 0.2f);

		// Stop all forces
		CEntity* player = getPlayer();
		TMsgRemoveForces msgForce;
		msgForce.byPlayer = false;
		msgForce.force_origin = "Cygnus";
		player->sendMsg(msgForce);

		EngineLua.executeScript("dispatchEvent('Gameplay/Cygnus/Phase_1_to_2')", 0.7f);

#else
		// To avoid playing cinematics
		TaskUtils::resumeAction(ctx, name);
		ctx.setIsDying(true);

		// Stop all forces
		CEntity* player = getPlayer();
		TMsgRemoveForces msgForce;
		msgForce.byPlayer = false;
		msgForce.force_origin = "Cygnus";
		player->sendMsg(msgForce);

		CEntity* e = ctx.getOwnerEntity();
		TCompTransform* transform = e->get<TCompTransform>();

		// Get Form 1 info
		CTransform t;
		t.fromMatrix(*transform);
		float yaw = transform->getYawRotationToAimTo(player->getPosition());
		t.setRotation(QUAT::Concatenate(QUAT::CreateFromYawPitchRoll(yaw, 0.f, 0.f), t.getRotation()));

		// Destroy form 1 entity
		ctx.getOwnerEntity().destroy();
		CHandleManager::destroyAllPendingObjects();

		// Enable BT
		CEntity* e_owner = spawn("data/prefabs/cygnus_form_2.json", t);
		TCompBT* c_bt = e_owner->get<TCompBT>();
		assert(c_bt);
		c_bt->setEnabled(true);

		// Show health bar
		TCompHealth* c_health = e_owner->get<TCompHealth>();
		c_health->setRenderActive(true, "cygnus");
#endif
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		return tickCondition(ctx, "is_dead", dt);
	}
};

class CBTTaskCygnusTimeReversal : public IBTTask
{
private:
	TCompAITimeReversal* c_time_reversal;

public:
	void init() override {
	}

	void onEnter(CBTContext& ctx) override {

		c_time_reversal = ctx.getComponent<TCompAITimeReversal>();
		c_time_reversal->startRewinding();
		ctx.allowAborts(false);


		// FMOD event, as there is no animation bound to the task
		TCompTransform* h_trans = ctx.getComponent<TCompTransform>();
		EngineAudio.postEvent("CHA/Cygnus/P2/AT/Cygnus_P2_Time_Reversal", h_trans->getPosition());
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		if (c_time_reversal->isRewinding())
			return EBTNodeResult::IN_PROGRESS;

		ctx.allowAborts(true);
		return EBTNodeResult::SUCCEEDED;
	}
};

// Teleport to a position defined in the blackboard
class CBTTaskCygnusTeleport : public IBTTask
{
private:
	float initial_hole_scale = 0.16f;
	float max_hole_scale = 2.5f;
	float damp_speed = 8.f;

	bool teleport_to_entity = false;

public:
	void init() override {

		teleport_to_entity = bool_field;

		callbacks.onFirstFrame = [&](CBTContext& ctx, float dt)
		{
			// Disable colliders
			TCompCollider* c_collider = ctx.getComponent<TCompCollider>();
			c_collider->disable(true);
		};

		callbacks.onStartup = [&](CBTContext& ctx, float dt)
		{
			TCompParent* parent = ctx.getComponent<TCompParent>();
			CEntity* hole = parent->getChildByName("Cygnus_black_hole");
			TCompAttachedToBone* socket = hole->get<TCompAttachedToBone>();
			CTransform& t = socket->getLocalTransform();
			t.setScale(damp<VEC3>(t.getScale(), VEC3(max_hole_scale), damp_speed, dt));
		};

		callbacks.onStartupFinished = [&](CBTContext& ctx, float dt)
		{
			TCompRender* render = ctx.getComponent<TCompRender>();
			render->setEnabled(false);
			// Stop the animation and return to Locomotion state
			TaskUtils::stopAction(ctx.getOwnerEntity(), "cygnus_f2_heal", 0.1f);

			// Remove lock on..
			CEntity* e_player = getEntityByName("player");
			TCompPlayerController* c_player_cont = e_player->get<TCompPlayerController>();
			if (c_player_cont->is_locked_on)
				c_player_cont->removeLockOn();
		};

		callbacks.onActive = [&](CBTContext& ctx, float dt)
		{
			TCompParent* parent = ctx.getComponent<TCompParent>();
			CEntity* hole = parent->getChildByName("Cygnus_black_hole");
			TCompAttachedToBone* socket = hole->get<TCompAttachedToBone>();
			CTransform& t = socket->getLocalTransform();
			t.setScale(damp<VEC3>(t.getScale(), VEC3::Zero, damp_speed, dt));
		};

		callbacks.onActiveFinished = [&](CBTContext& ctx, float dt)
		{
			// Re-enable colliders
			TCompCollider* c_collider = ctx.getComponent<TCompCollider>();
			c_collider->disable(false);
			
			VEC3 target_pos;
			QUAT target_rot;

			// Get the position of the entity or the one stored in the BB key according to the player
			if (teleport_to_entity) {
				CEntity* e_target = getEntityByName(string_field);
				TCompTransform* c_trans_target = e_target->get<TCompTransform>();
				target_pos = c_trans_target->getPosition();
				target_rot = c_trans_target->getRotation();
			}
			else {
				// Get the rotation to look at Eon and the position behind or near Eon
				target_pos = ctx.getBlackboard()->getValue<VEC3>(string_field);
				CEntity* player = getPlayer();
				TCompTransform* h_trans_eon = player->get<TCompTransform>();
				target_rot = h_trans_eon->getRotation();
				
				// Check if the position is in the navmesh
				VEC3 eon_pos = h_trans_eon->getPosition();
				bool is_outside_bossarea = EngineNavMesh.raycast(target_pos, eon_pos);	// Returns true if it found an obstacle between the position and Eon
				bool has_obstacles = TaskUtils::hasObstaclesToEon(target_pos, h_trans_eon);
				if (is_outside_bossarea || has_obstacles) {
					target_pos = eon_pos + h_trans_eon->getForward() * 2;
					target_rot.Inverse(target_rot);
				}
			}
			
			// Set the position
			TCompCollider* h_collider = ctx.getComponent<TCompCollider>();
			h_collider->setFootPosition(target_pos);
			
			// Set the rotation
			TCompTransform* h_trans = ctx.getComponent<TCompTransform>();
			TCompAIControllerBase* h_aicontroller = ctx.getComponent<TCompAIControllerBase>();
			h_aicontroller->setTargetRotation(target_rot);			// To avoid the rotation applied by the AI Controller
			h_trans->setRotation(target_rot);
		};

		callbacks.onRecovery = [&](CBTContext& ctx, float dt)
		{
			TCompParent* parent = ctx.getComponent<TCompParent>();
			CEntity* hole = parent->getChildByName("Cygnus_black_hole");
			TCompAttachedToBone* socket = hole->get<TCompAttachedToBone>();
			CTransform& t = socket->getLocalTransform();

			TCompRender* render = ctx.getComponent<TCompRender>();
			bool render_ok = render->draw_calls[0].enabled;

			if (!render_ok)
			{
				t.setScale(damp<VEC3>(t.getScale(), VEC3(max_hole_scale), damp_speed, dt));
				if (VEC3::Distance(t.getScale(), VEC3(max_hole_scale)) < 0.01f)
				{
					TCompRender* render = ctx.getComponent<TCompRender>();
					render->setEnabled(true);
				}
			}
			else
			{
				t.setScale(damp<VEC3>(t.getScale(), VEC3(initial_hole_scale), damp_speed, dt));
				if (VEC3::Distance(t.getScale(), VEC3(initial_hole_scale)) < 0.01f)
				{
					ctx.setFSMVariable("is_teleporting", 0);
				}
			}
		};
	}

	void onEnter(CBTContext& ctx) override {
		
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		return tickCondition(ctx, "is_teleporting", dt, false);
	}
};

class CBTTaskCygnusChangePhase : public IBTTask
{
private:
	int phase_num = 2;

public:
	void init() override {
		phase_num = (int)number_field[0];
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {		
		clamp(phase_num, 2, 4);
		ctx.setFSMVariable("phase_number", phase_num);
		ctx.getBlackboard()->setValue<int>("phaseNumber", phase_num);

		// Always remove emissive pinxos on phase change
		CEntity* owner = ctx.getOwnerEntity();
		TCompEmissiveMod* mod = owner->get<TCompEmissiveMod>();
		if (mod)
			mod->blendOut();

		if (phase_num != 3)
			return EBTNodeResult::SUCCEEDED;

		// The only cinematic is from F2 to F3 (in phase 3)
		EngineUI.fadeOut(0.7f, 0.2f, 0.2f);
			
		TCompBT* c_bt = ctx.getComponent<TCompBT>();
		assert(c_bt);
		c_bt->setEnabled(false);

		// Hide health bar
		TCompHealth* c_health = ctx.getComponent<TCompHealth>();
		c_health->setRenderActive(false);

		EngineLua.executeScript("dispatchEvent('Gameplay/Cygnus/Phase_2_to_3')", 0.7f);
		
		return EBTNodeResult::SUCCEEDED;
	}
};

class CBTTaskCygnusBeamAttack : public IBTTask
{
private:
	int damage;
	float max_dep_angle;
	float move_speed;
	float dist_to_eon;
	CHandle h_beam;
	CHandle h_attract_particles;
	
	// The beam will start from the black hole and look at a changing target
	

public:
	void init() override {
		// Load generic parameters
		damage = (int)number_field[0];
		max_dep_angle = deg2rad(number_field[1]);			// Change the depression angle to radian
		move_speed = number_field[2];

		callbacks.onFirstFrame = [&](CBTContext& ctx, float dt)
		{
			TCompTransform* h_trans = ctx.getComponent<TCompTransform>();
			CEntity* e_cygnus = ctx.getOwnerEntity();
			VEC3 hole_pos = TaskUtils::getBoneWorldPosition(e_cygnus, "cygnus_hole_jnt");
			h_attract_particles = spawnParticles("data/particles/compute_cygnus_ray_base_absorb_particles.json", hole_pos, hole_pos);
		};

		callbacks.onStartup = [&](CBTContext& ctx, float dt)
		{
			// Get the black hole position
			CEntity* e_cygnus = ctx.getOwnerEntity();
			VEC3 black_hole_pos = TaskUtils::getBoneWorldPosition(e_cygnus, "cygnus_hole_jnt");

			if (!h_attract_particles.isValid()) {
				return;
			}

			// Attract particles
			CEntity* e_attract_particles = h_attract_particles;
			TCompBuffers* c_buff = e_attract_particles->get<TCompBuffers>();
			CShaderCte< CtesParticleSystem >* cte = static_cast<CShaderCte<CtesParticleSystem>*>(c_buff->getCteByName("CtesParticleSystem"));
			cte->emitter_initial_pos = black_hole_pos;
			cte->updateFromCPU();

			// Scale black hole
			TCompParent* parent = ctx.getComponent<TCompParent>();
			CEntity* hole = parent->getChildByName("Cygnus_black_hole");
			TCompAttachedToBone* socket = hole->get<TCompAttachedToBone>();
			CTransform& t = socket->getLocalTransform();
			t.setScale(damp<VEC3>(t.getScale(), VEC3(0.6f), 4.f, dt));
		};

		callbacks.onStartupFinished = [&](CBTContext& ctx, float dt)
		{
			ctx.setNodeVariable(name, "allow_aborts", false);

			// Get the black hole position
			CEntity* e_cygnus = ctx.getOwnerEntity();
			VEC3 black_hole_pos = TaskUtils::getBoneWorldPosition(e_cygnus, "cygnus_hole_jnt");

			// Get the initial beam target rotating the Cygnus forward with a pitch angle
			TCompTransform* h_trans = ctx.getComponent<TCompTransform>();
			VEC3 cygnus_forward = h_trans->getForward();
			VEC3 beam_target_dir = DirectX::XMVector3Rotate(cygnus_forward, QUAT::CreateFromAxisAngle(VEC3(0.0f, 1.0f, 0.0f), max_dep_angle));
			ctx.setNodeVariable(name, "beam_target_dir", beam_target_dir);

			// Place the beam in the black hole, looking at the target
			CTransform clone_trans;
			clone_trans.setPosition(black_hole_pos);
			clone_trans.lookAt(black_hole_pos, beam_target_dir + black_hole_pos, VEC3::Up);
			h_beam = spawn("data/prefabs/cygnus_beam.json", clone_trans);

			// Set beam parameters
			CEntity* e_beam = h_beam;
			TCompCygnusBeam* c_cygnusbeam = e_beam->get<TCompCygnusBeam>();
			c_cygnusbeam->setParameters(damage);
		};

		// Set animation callbacks
		callbacks.onActive = [&](CBTContext& ctx, float dt)
		{
			// Get the black hole position
			CEntity* e_cygnus = ctx.getOwnerEntity();
			VEC3 black_hole_pos = TaskUtils::getBoneWorldPosition(e_cygnus, "cygnus_hole_jnt");

			if (h_attract_particles.isValid()) {
				CEntity* e_attract_particles = h_attract_particles;
				TCompBuffers* c_buff = e_attract_particles->get<TCompBuffers>();
				CShaderCte< CtesParticleSystem >* cte = static_cast<CShaderCte<CtesParticleSystem>*>(c_buff->getCteByName("CtesParticleSystem"));
				cte->emitter_initial_pos = black_hole_pos;
				cte->updateFromCPU();
			}

			// Get beam dir, target and origin from node variables
			float beam_dir = ctx.getNodeVariable<float>(name, "beam_dir");
			VEC3 beam_target_dir = ctx.getNodeVariable<VEC3>(name, "beam_target_dir");

			// Change the beam direction depending on the animation time
			float dt_acum = ctx.getNodeVariable<float>(name, "dt_acum");
			dt_acum += dt;
			ctx.setNodeVariable(name, "dt_acum", dt_acum);

			float speed_smooth = 0.0f;
			if (dt_acum < 1.16f) {
				speed_smooth = move_speed * interpolators::cubicInOut(0.0, 1.0, dt_acum / 1.16f);
			} else
			if (dt_acum >= 1.16f && dt_acum < 2.66f) {
				ctx.setNodeVariable(name, "beam_dir", 1.f);
				speed_smooth = move_speed * interpolators::cubicInOut(0.0, 1.0, (dt_acum - 1.16f) / (2.66f - 1.16f));

			} else
			if (dt_acum >= 2.66f) {
				ctx.setNodeVariable(name, "beam_dir", -1.f);
				speed_smooth = move_speed * interpolators::cubicInOut(0.0, 1.0, (dt_acum - 2.66f) / (3.0f - 2.66f));
			}

			// Calculate the speed and rotation, and store the new beam target
			float delta_yaw = deg2rad(speed_smooth * 8) * dt * beam_dir;
			
			//float delta_pitch = max_dep_angle * dt * 0.25f;
			//beam_target_dir	= DirectX::XMVector3Rotate(beam_target_dir, QUAT::CreateFromYawPitchRoll(delta_yaw, -delta_pitch, 0.f));
			
			beam_target_dir	= DirectX::XMVector3Rotate(beam_target_dir, QUAT::CreateFromYawPitchRoll(delta_yaw, 0.f, 0.f));
			beam_target_dir = damp(beam_target_dir, VEC3::Down, 0.05f, dt);
			ctx.setNodeVariable(name, "beam_target_dir", beam_target_dir);

			// Rotate the beam to look at the target
			CEntity* e_beam = h_beam;
			TCompTransform* c_trans = e_beam->get<TCompTransform>();
			c_trans->lookAt(black_hole_pos, beam_target_dir + black_hole_pos, VEC3::Up);
			
			// Reduce the scale so that it doesn't cut abruptly
			VEC3 new_scale = c_trans->getScale() - 0.25f * VEC3(dt, dt, 0);
			new_scale.Clamp(VEC3::Zero, VEC3::One);
			c_trans->setScale(new_scale);

			std::vector<physx::PxRaycastHit> raycastHits;
			beam_target_dir.Normalize();
			if (EnginePhysics.raycast(black_hole_pos + beam_target_dir, beam_target_dir, 20.f, raycastHits, CModulePhysics::FilterGroup::Scenario | CModulePhysics::FilterGroup::Characters, true, true)) {
				VEC3 coll_pos = PXVEC3_TO_VEC3(raycastHits.front().position);
				VEC3 coll_normal = PXVEC3_TO_VEC3(raycastHits.front().normal);
				coll_normal.Normalize();
				spawnParticles("data/particles/compute_cygnus_ray_particles.json", coll_pos + coll_normal * 0.1f, coll_pos);
			}
		};

		callbacks.onActiveFinished = [&](CBTContext& ctx, float dt)
		{
			// Destroy the beam after the animation has ended
			h_beam.destroy();

			ctx.setNodeVariable(name, "allow_aborts", true);
		};

		callbacks.onRecovery = [&](CBTContext& ctx, float dt)
		{
			// Scale black hole
			TCompParent* parent = ctx.getComponent<TCompParent>();
			CEntity* hole = parent->getChildByName("Cygnus_black_hole");
			TCompAttachedToBone* socket = hole->get<TCompAttachedToBone>();
			CTransform& t = socket->getLocalTransform();
			t.setScale(damp<VEC3>(t.getScale(), VEC3(0.16f), 8.f, dt));
		};
	}

	// Executed on first frame
	void onEnter(CBTContext& ctx) override {
		ctx.setNodeVariable(name, "dt_acum", 0.f);
		ctx.setNodeVariable(name, "beam_dir", -1.f);
		ctx.setNodeVariable(name, "allow_aborts", true);
		ctx.setNodeVariable(name, "beam_target_dir", VEC3::Zero);
		ctx.setNodeVariable(name, "black_hole_pos", VEC3::Zero);
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		return tickCondition(ctx, "is_ranged_attacking", dt, ctx.getNodeVariable<bool>(name, "allow_aborts"));
	}
};

class CBTTaskCygnusSpawnClones : public IBTTask
{
private:
	float period = 1.f;
	float clone_lifespan = 10.f;
	float initial_hole_scale = 0.16f;
	float max_hole_scale = 1.f;
	float damp_speed = 6.f;

public:
	void init() override {
		// Load generic parameters
		period = number_field[0];
		clone_lifespan = number_field[1];

		callbacks.onStartupFinished = [&](CBTContext& ctx, float dt)
		{
			ctx.setNodeVariable(name, "allow_aborts", false);
		};

		// Set animation callbacks
		callbacks.onActive = [&](CBTContext& ctx, float dt)
		{
			// Spawn enemies when time is 0
			float dt_acum = ctx.getNodeVariable<float>(name, "dt_acum");
			
			// Accumulate time
			dt_acum += dt;

			// dt_acum = period -> 
			float pct = clampf(dt_acum / period, 0.f, 1.f);
			printFloat("PCT", pct);
			TCompParent* parent = ctx.getComponent<TCompParent>();
			CEntity* hole = parent->getChildByName("Cygnus_black_hole");
			TCompAttachedToBone* socket = hole->get<TCompAttachedToBone>();
			CTransform& t = socket->getLocalTransform();
			t.setScale(damp<VEC3>(t.getScale(), VEC3(initial_hole_scale + max_hole_scale * pct), damp_speed, dt));
			printVEC3(t.getScale());

			if (dt_acum >= period)
			{
				dt_acum = 0.f;
				VEC3 pos = TaskUtils::getBoneWorldPosition(ctx.getOwnerEntity(), "cygnus_hole_jnt");
				TaskUtils::spawnCygnusForm1Clone(pos, clone_lifespan);
			}

			ctx.setNodeVariable(name, "dt_acum", dt_acum);
		};

		callbacks.onActiveFinished = [&](CBTContext& ctx, float dt)
		{
			ctx.setNodeVariable(name, "allow_aborts", true);
			ctx.setNodeVariable(name, "dt_acum", 0.f);
		};

		callbacks.onRecovery = [&](CBTContext& ctx, float dt)
		{
			TCompParent* parent = ctx.getComponent<TCompParent>();
			CEntity* hole = parent->getChildByName("Cygnus_black_hole");
			TCompAttachedToBone* socket = hole->get<TCompAttachedToBone>();
			CTransform& t = socket->getLocalTransform();
			t.setScale(damp<VEC3>(t.getScale(), VEC3(initial_hole_scale), damp_speed, dt));
		};
	}

	// Executed on first frame
	void onEnter(CBTContext& ctx) override {
		ctx.setNodeVariable(name, "dt_acum", 0.f);
		ctx.setNodeVariable(name, "allow_aborts", true);
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		return tickCondition(ctx, "is_spawning_enemies", dt, ctx.getNodeVariable<bool>(name, "allow_aborts"));
	}
};

class CBTTaskCygnusCloneDeath : public IBTTask
{
public:
	void init() override {}

	void onEnter(CBTContext& ctx) override {
		ctx.setIsDying(true);
		TaskUtils::dissolveAt(ctx, 5.f, 0.1f);

		CEntity* owner = ctx.getOwnerEntity();

		// Destroy entity
		std::string name = owner->getName();
		std::string argument = "destroyEntity('" + name + "')";

		// sync with dissolve time
		EngineLua.executeScript(argument, 5.f);
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		return tickCondition(ctx, "is_dead", dt);
	}
};

class CBTTaskCygnusFinalDeath : public IBTTask
{
public:
	void init() override {}

	void onEnter(CBTContext& ctx) override {
		ctx.setIsDying(true);

		CEntity* e = ctx.getOwnerEntity();

		EngineUI.fadeOut(0.7f, 0.2f, 0.2f);

		// Hide health bar
		TCompHealth* c_health = e->get<TCompHealth>();
		c_health->setRenderActive(false);

		EngineLua.executeScript("dispatchEvent('Gameplay/Cygnus/FinalDeath')", 0.7f);
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		return tickCondition(ctx, "is_dead", dt);
	}
};

#pragma endregion

#pragma region General Enemies Tasks
class CBTTaskHealing : public IBTTask
{

public:
	void init() override {
		// Load generic parameters

		callbacks.onFirstFrame = [&](CBTContext& ctx, float dt)
		{
			TCompTransform* c_trans = ctx.getComponent<TCompTransform>();
			TCompName* c_name = ctx.getComponent<TCompName>();
			PawnUtils::spawnHealParticles(c_trans->getPosition(), c_name->getName());
			ctx.setNodeVariable(name, "allow_aborts", false);
		};

		callbacks.onStartupFinished = [&](CBTContext& ctx, float dt)
		{
			ctx.setNodeVariable(name, "allow_aborts", false);
		};

		// Heal only after the animation ends
		callbacks.onActiveFinished = [&](CBTContext& ctx, float dt)
		{
			ctx.setNodeVariable(name, "allow_aborts", true);

			TCompHeal* h_heal = ctx.getComponent<TCompHeal>();
			h_heal->heal();
		};
	}

	void onEnter(CBTContext& ctx) override {
		ctx.setNodeVariable(name, "allow_aborts", true);
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		return tickCondition(ctx, "is_healing", dt, ctx.getNodeVariable<bool>(name, "allow_aborts"));
	}
};

class CBTTaskHitstunned : public IBTTask
{
public:
	void init() override {}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		EBTNodeResult res;

		TaskUtils::resumeAction(ctx, name);						// Resume any paused animation, if applicable

		bool isStrongHitstun = ctx.getBlackboard()->getValue<bool>("strongDamage");

		if (ctx.getBlackboard()->hasKey("isHitstunBack") && ctx.getBlackboard()->getValue<bool>("isHitstunBack")) {
			res = tickCondition(ctx, "is_hitstunned_back", dt);
		}
		// If there was a strong hit, play the animation and restore the value of "strongDamage" to false when it finishes
		else if (isStrongHitstun) {
			res = tickCondition(ctx, "is_hitstunned_strong", dt); 
			if (res == EBTNodeResult::SUCCEEDED)
				ctx.getBlackboard()->setValue("strongDamage", false);
		}
		else {
			res = tickCondition(ctx, "is_hitstunned", dt);

			// Check if another hit has been received
			int processHitCount = ctx.getBlackboard()->getValue<int>("processHit");

			if (processHitCount > 0) {
				ctx.getBlackboard()->setValue("processHit", processHitCount - 1);
				ctx.setFSMVariable("is_hitstunned", 1 + (processHitCount % 2));
			}
		}

		return res;
	}
};

class CBTTaskStunned : public IBTTask
{
public:
	void init() override {}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		return tickCondition(ctx, "is_stunned", dt);
	}
};

class CBTTaskEnemyDeath : public IBTTask
{
public:
	void init() override {}

	void onEnter(CBTContext& ctx) override {
		ctx.setIsDying(true);
		TaskUtils::resumeAction(ctx, name);
		TaskUtils::dissolveAt(ctx, 18.f, 0.5f);

		CEntity* owner = ctx.getOwnerEntity();
		// Change group to avoid new hits (blood, etc)
		TCompCollider* collider = owner->get<TCompCollider>();
		assert(collider);
		collider->setGroupAndMask("invisible_wall", "all");

		TCompForceReceiver* force = owner->get<TCompForceReceiver>();
		if (force) {
			owner->removeComponent<TCompForceReceiver>();
		}
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		return tickCondition(ctx, "is_dying", dt);
	}
};

#pragma endregion

#pragma region Generic Tasks
class CBTTaskWait : public IBTTask
{
private:
	float seconds;

public:
	void init() override {
		// Load generic parameters
		seconds = number_field[0];
	}

	void onEnter(CBTContext& ctx) override {
		ctx.setNodeVariable(name, "dt_acum", 0.f);
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		// Wait for number_field seconds

		float dt_acum = ctx.getNodeVariable<float>(name, "dt_acum");
		dt_acum += dt;
		ctx.setNodeVariable(name, "dt_acum", dt_acum);

		if (dt_acum < seconds) {
			return EBTNodeResult::IN_PROGRESS;
		}
		else {
			return EBTNodeResult::SUCCEEDED;
		}
	}
};

class CBTTaskSetBlackboardBool : public IBTTask
{
private:
	std::string key;
	bool value;

public:
	void init() override {
		// Load generic parameters
		key = string_field;
		value = bool_field;
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		// Set boolean variable
		bool is_ok = ctx.getBlackboard()->setValue(key, value);
		if (is_ok)
			return EBTNodeResult::SUCCEEDED;
		else
			return EBTNodeResult::FAILED;
	}
};

class CBTTaskSetBlackboardFloat : public IBTTask
{
private:
	std::string key;
	float value;

public:
	void init() override {
		// Load generic parameters
		key = string_field;
		value = number_field[0];
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		// Set float variable
		bool is_ok = ctx.getBlackboard()->setValue(key, value);
		if (is_ok)
			return EBTNodeResult::SUCCEEDED;
		else
			return EBTNodeResult::FAILED;
	}
};

class CBTTaskSetBlackboardInt : public IBTTask
{
private:
	std::string key;
	int value;

public:
	void init() override {
		// Load generic parameters
		key = string_field;
		value = (int)number_field[0];
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		// Set int variable
		bool is_ok = ctx.getBlackboard()->setValue(key, value);
		if (is_ok)
			return EBTNodeResult::SUCCEEDED;
		else
			return EBTNodeResult::FAILED;
	}
};

class CBTTaskDecrementBlackboardInt : public IBTTask
{
private:
	std::string key;
	int decr_val;

public:
	void init() override {
		// Load generic parameters
		key = string_field;
		decr_val = static_cast<int>(number_field[0]);
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		// Decrement int variable
		int cur_val = ctx.getBlackboard()->getValue<int>(key);
		bool is_ok = ctx.getBlackboard()->setValue(key, cur_val - decr_val);
		if (is_ok)
			return EBTNodeResult::SUCCEEDED;
		else
			return EBTNodeResult::FAILED;
	}
};

class CBTTaskPlayAnimation : public IBTTask
{
private:
	std::string animation_name;
	bool is_cycle = false;

public:
	void init() override {
		// Load generic parameters
		animation_name = string_field;
		is_cycle = bool_field;
	}

	void onEnter(CBTContext& ctx) override {
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		CEntity* e_owner = ctx.getOwnerEntity();

		if (is_cycle) {
			TaskUtils::playCycle(e_owner, animation_name);
		}
		else {
			TaskUtils::playAction(e_owner, animation_name);
		}

		return EBTNodeResult::SUCCEEDED;
	}
};

class CBTTaskFMODSetGlobalRTPC : public IBTTask
{
private:
	std::string rtpc_name;
	float value;

public:
	void init() override {
		// Load generic parameters
		rtpc_name = string_field;
		value = number_field[0];
	}

	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		bool is_ok = EngineAudio.setGlobalRTPC(rtpc_name, value);

		if (is_ok)
			return EBTNodeResult::SUCCEEDED;
		else
			return EBTNodeResult::FAILED;
	}
};


#pragma endregion

#pragma region Debugging Tasks
/////////////////////////////////
//////// DEBUGGING TASKS ////////
/////////////////////////////////

class CBTTaskSuccess : public IBTTask
{
public:
	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		return EBTNodeResult::SUCCEEDED;
	}
};

class CBTTaskFailure : public IBTTask
{
public:
	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		return EBTNodeResult::FAILED;
	}
};

class CBTTaskInProgress : public IBTTask
{
public:
	EBTNodeResult executeTask(CBTContext& ctx, float dt) {
		return EBTNodeResult::IN_PROGRESS;
	}
};

#pragma endregion

#define REGISTER_TASK(TASK_NAME)  {#TASK_NAME, new CBTTaskFactory<CBTTask##TASK_NAME>()},
std::map<std::string_view, CBTParser::IBTTaskFactory*> CBTParser::task_types{
	#include "bt_task_registration.h"
};
#undef REGISTER_TASK