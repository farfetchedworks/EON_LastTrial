#include "mcv_platform.h"
#include "bt_parser.h"
#include "bt_context.h"
#include "bt.h"
#include "task_utils.h"
#include "components/common/comp_name.h"
#include "components/common/comp_transform.h"
#include "components/controllers/comp_pawn_controller.h"
#include "components/controllers/pawn_utils.h"
#include "components/controllers/comp_ai_controller_base.h"
#include "skeleton/comp_skel_lookat.h"
#include "engine.h"
#include "modules/module_physics.h"

/*
*	Declare and implement all services here
*	The macros at the bottom will register each of them
*/

class CBTServiceDistanceSquaredToPlayer : public IBTService
{
private:
	CHandle h_trans;
	CHandle h_player_trans;

public:
	void init() override {}

	EBTNodeResult executeService(CBTContext& ctx, float dt) {

		TCompTransform* h_trans = ctx.getComponent<TCompTransform>();
		
		CEntity* h_player = TaskUtils::getPlayer();
		if (!h_player) return EBTNodeResult::FAILED;

		TCompTransform* h_player_trans = h_player->get<TCompTransform>();

		float distance = VEC3::DistanceSquared(h_trans->getPosition(), h_player_trans->getPosition());

		//dbg("Running service: DistanceSquaredToPlayer: %f\n", distance);

		ctx.getBlackboard()->setValue(string_field, distance);

		return EBTNodeResult::SUCCEEDED;
	}
};

// Set if player was detected
class CBTServiceSetPlayerDetected : public IBTService
{
private:
	float max_chase_distance = 0.f;
	std::string bb_keyname_is_detected = "";

	const std::string bb_keyname_distance = "distanceSquared";

public:
	void init() override {
		// Load generic parameters
		max_chase_distance = number_field;
		bb_keyname_is_detected = string_field;
	}

	EBTNodeResult executeService(CBTContext& ctx, float dt) {

		CEntity* h_player = TaskUtils::getPlayer();
		bool player_was_detected = ctx.getBlackboard()->getValue<bool>(bb_keyname_is_detected);
		
		// Always get distance from blackboard (execute after the previous service)
		float distance = ctx.getBlackboard()->getValue<float>(bb_keyname_distance);

		// If player is in the chase distance, check if it sees the player
		if (distance < max_chase_distance) {

			// Check if player is inside cone only if the player has not been detected before
			if (!player_was_detected) {
				TCompAIControllerBase* h_controller = ctx.getComponent<TCompAIControllerBase>();
				TCompSkelLookAt* h_skellookat = ctx.getComponent<TCompSkelLookAt>();

				// If the entity has a look at bone, use it to detect the player
				bool detected = (h_skellookat && h_skellookat->head_bone_name != "") ? 
					PawnUtils::isInsideConeOfBone(ctx.getOwnerEntity(), h_player, h_skellookat->head_bone_name, h_controller->fov_rad, h_controller->sight_radius) :
					PawnUtils::isInsideCone(ctx.getOwnerEntity(), h_player, h_controller->fov_rad, h_controller->sight_radius);
				bool has_heard = PawnUtils::hasHeardPlayer(ctx.getOwnerEntity(), h_controller->hearing_radius, h_controller->hear_speed_th);

				if (detected || has_heard) {
					// If it only detects sight
					detected &= TaskUtils::canSeePlayer(ctx.getComponent<TCompTransform>(), h_player->get<TCompTransform>());

					if (detected || has_heard) {
						ctx.getBlackboard()->setValue<bool>(bb_keyname_is_detected, true);
						ctx.setHasEonTargeted(true);

						// Send a message to the player that he has been detected -targeted-, for music-interaction purposes
						TMsgTarget h_msg;
						h_player->sendMsg(h_msg);
					}
				}
			}
		}
		else {
			// If the player is far away and player was already detected, set the key name to false
			if (player_was_detected) {
				ctx.getBlackboard()->setValue<bool>(bb_keyname_is_detected, false);
				ctx.setHasEonTargeted(false);

				// Send a message to the player that he has been untargeted, for music-interaction purposes
				TMsgUntarget h_msg;
				h_player->sendMsg(h_msg);
			}
		}

		return EBTNodeResult::SUCCEEDED;
	}
};

// Decrement cooldown
class CBTServiceCooldown : public IBTService
{
private:
	std::string bb_key_cooldown;

public:
	void init() override {
		// Load generic parameters
		bb_key_cooldown = string_field;
	}

	EBTNodeResult executeService(CBTContext& ctx, float dt) {
		
		float CD = ctx.getBlackboard()->getValue<float>(bb_key_cooldown);

		float accum_cooldown = ctx.getNodeVariable<float>(name, "period_accum");
		accum_cooldown > CD ? CD = 0 : CD -= accum_cooldown; // decrement CD by time since last time this service was executed
		ctx.getBlackboard()->setValue<float>(bb_key_cooldown, CD);

		return EBTNodeResult::SUCCEEDED;
	}
};

/*
* Set where the player is relative to the AI: in front, in the back, left or right
* Each bit is associated to a relative position:
* 1111 - left-right-back-front
*/

// 
class CBTServiceSetPlayerPosition : public IBTService
{
private:
	std::string bb_keyname_player_pos = "";
	float sphere_radius = 1.f;
	int pos_mask = 0;

public:
	void init() override {
		// Load generic parameters
		bb_keyname_player_pos = string_field;
		sphere_radius = number_field;
	}

	EBTNodeResult executeService(CBTContext& ctx, float dt) {

		pos_mask = 0;
		physx::PxU32 layer_mask = CModulePhysics::FilterGroup::Player;
		VHandles overlapped_objects;

		TCompTransform* c_trans = ctx.getComponent<TCompTransform>();

		// Is on the front
		if (EnginePhysics.overlapSphere(c_trans->getPosition()+c_trans->getForward(), sphere_radius, overlapped_objects, layer_mask)) {
			pos_mask |= 1 << 0;
		}
		// Is on the back
		if (EnginePhysics.overlapSphere(c_trans->getPosition() + (-1)*c_trans->getForward(), sphere_radius, overlapped_objects, layer_mask)) {
			pos_mask |= 1 << 1;
		}

		// Is on the right
		if (EnginePhysics.overlapSphere(c_trans->getPosition() + c_trans->getRight(), sphere_radius, overlapped_objects, layer_mask)) {
			pos_mask |= 1 << 2;
		}

		// Is on the left
		if (EnginePhysics.overlapSphere(c_trans->getPosition() + (-1) * c_trans->getRight(), sphere_radius, overlapped_objects, layer_mask)) {
			pos_mask |= 1 << 3;
		}

		ctx.getBlackboard()->setValue<int>(bb_keyname_player_pos, pos_mask);

		return EBTNodeResult::SUCCEEDED;
	}
};

#define REGISTER_SERVICE(SERVICE_NAME)  {#SERVICE_NAME, new CBTServiceFactory<CBTService##SERVICE_NAME>()},
std::map<std::string_view, CBTParser::IBTServiceFactory*> CBTParser::service_types{
	#include "bt_service_registration.h"
};
#undef REGISTER_TASK