#include "mcv_platform.h"
#include "handle/handle.h"
#include "engine.h"
#include "modules/subvert/module_player_interaction.h"
#include "audio/module_audio.h"
#include "components/gameplay/comp_energy_wall.h"
#include "components/common/comp_transform.h"
#include "components/controllers/comp_player_controller.h"
#include "components/abilities/comp_time_reversal.h"
#include "components/common/comp_collider.h"
#include "components/messages.h"

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
	PlayerInteraction.setPlaying(false);

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

	is_active = true;
	eon_passed = true;
	
	TCompPlayerController* c_controller = e_player->get<TCompPlayerController>();
	c_controller->blendCamera("camera_interact", 4.0f, &interpolators::cubicInOutInterpolator);

	// disable collisions with the player
	TCompCollider* c_collider = get<TCompCollider>();
	c_collider->setGroupAndMask("interactable", "all_but_player");

	// hack to allow pass through the wall when Eon is very close
	TCompTransform* trans = get<TCompTransform>();
	VEC3 init_pos = trans->getPosition();

	TCompTransform* player_transform = e_player->get<TCompTransform>();
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

	VEC3 offset = trans->getForward() * 0.1f;
	VEC3 wall_pos = trans->getPosition();
	wall_pos.y = player_transform->getPosition().y;

	VEC3 entry_point = wall_pos - offset;

	float dis_wall = VEC3::DistanceSquared(trans->getPosition(), player_transform->getPosition());
	float dis_entry = VEC3::DistanceSquared(entry_point, player_transform->getPosition());

	if (dis_entry > dis_wall) {
		entry_point = trans->getPosition() + offset;
	}

	return entry_point;
}