#include "mcv_platform.h"
#include "handle/handle.h"
#include "engine.h"
#include "modules/game/module_player_interaction.h"
#include "audio/module_audio.h"
#include "components/gameplay/comp_energy_wall.h"
#include "components/common/comp_transform.h"
#include "components/controllers/comp_player_controller.h"
#include "components/abilities/comp_time_reversal.h"
#include "components/common/comp_collider.h"
#include "components/messages.h"
#include "modules/module_physics.h"

DECL_OBJ_MANAGER("energy_wall", TCompEnergyWall)

void TCompEnergyWall::update(float dt)
{
	if (!is_active)
		return;

	TCompTransform* c_trans = get<TCompTransform>();
	TCompTransform* player_transform = e_player->get<TCompTransform>();
	TCompPlayerController* c_controller = e_player->get<TCompPlayerController>();

	c_controller->last_dir = dir_entry_point;

	float dis_entry = VEC3::DistanceSquared(entry_point, player_transform->getPosition());
	if (dis_entry > 0.5f && is_moving) {

		// move player to entry point
		VEC3 move = dir_entry_point * move_speed * dt;
		c_controller->movePhysics(move, dt);
		return;
	}

	is_moving = false;

	if (curr_time == 0.f)
		c_controller->setVariable("is_crossing_wall", true);
	
	c_controller->move(dt);

	curr_time += dt;

	if (curr_time < move_time)
		return;

	is_active = false;
	curr_time = 0.f;
	PlayerInteraction.setActive(false);

	// enable collisions
	TCompCollider* c_collider = get<TCompCollider>();
	c_collider->setGroupAndMask("interactable", "all");

	// to avoid Eon exiting the energy wall using time reversal
	TCompTimeReversal* c_time_reversal = e_player->get<TCompTimeReversal>();
	c_time_reversal->clearBuffer();
}

/*
	Messages callbacks
*/

void TCompEnergyWall::onEonInteracted(const TMsgEonInteracted& msg)
{
	e_player = msg.h_player;

	TCompPawnController* controller = e_player->get<TCompPawnController>();
	if (eon_passed || controller->inAreaDelay())
		return;

	// Rotate Eon to look at the energy wall
	TCompTransform* player_transform = e_player->get<TCompTransform>();
	TCompTransform* trans = get<TCompTransform>();
	player_transform->setRotation(trans->getRotation());

	is_active = true;
	eon_passed = true;
	
	TCompPlayerController* c_controller = e_player->get<TCompPlayerController>();
	c_controller->blendCamera("camera_interact", 4.0f, &interpolators::cubicInOutInterpolator);

	// disable collisions with the player
	TCompCollider* c_collider = get<TCompCollider>();
	c_collider->setGroupAndMask("interactable", "all_but_player");

	// hack to allow pass through the wall when Eon is very close
	VEC3 init_pos = trans->getPosition();

	VEC3 new_pos = trans->getPosition() + trans->getForward() * 0.01f;
	trans->setPosition(new_pos);
	c_collider->setGlobalPose(trans->getPosition(), trans->getRotation());
	entry_point = calculateEntryPoint();

	dir_entry_point = entry_point - player_transform->getPosition();
	dir_entry_point.Normalize();
	is_moving = true;

	// post fmod event
	const static char* EVENT = "ENV/General/BossDoor/BossDoor_Interact";
	EngineAudio.postEvent(EVENT, init_pos);
}

VEC3 TCompEnergyWall::calculateEntryPoint()
{
	TCompTransform* trans = get<TCompTransform>();
	TCompTransform* player_transform = e_player->get<TCompTransform>();

	VEC3 player_pos = player_transform->getPosition();

	VEC3 offset = trans->getForward() * 0.1f;
	VEC3 wall_pos = trans->getPosition();

	// Generate a raycast to perform the animation where the player interacted
	std::vector<physx::PxRaycastHit> raycastHits;

	// Get the exact position of the wall the player is facing to perform the animation in that section
	bool is_ok = EnginePhysics.raycast(player_pos, -trans->getForward(), 20.f, raycastHits, CModulePhysics::FilterGroup::Interactable, true, false);
	if (is_ok) {
		wall_pos = PXVEC3_TO_VEC3(raycastHits.front().position);
	}

	wall_pos.y = player_transform->getPosition().y;

	VEC3 entry_point = wall_pos - offset;

	float dis_wall = VEC3::DistanceSquared(trans->getPosition(), player_pos);
	float dis_entry = VEC3::DistanceSquared(entry_point, player_pos);

	if (dis_entry > dis_wall) {
		entry_point = trans->getPosition() + offset;
	}

	return entry_point;
}