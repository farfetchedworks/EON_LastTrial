#pragma once

#include "components/common/comp_base.h"
#include "entity/entity.h"
#include "components/messages.h"

class TCompEnergyWall : public TCompBase {

	DECL_SIBLING_ACCESS();

private:

	bool eon_passed = false;			// set to true to avoid Eon exiting the energy wall area
	bool is_active = false;
	bool is_moving = false;
	bool is_facing_to_wall = false;
	float move_time = 3.f;				// time to move Eon through the wall
	float move_speed = 2.f;
	float curr_time = 0.f;
	CEntity* e_player;
	VEC3 entry_point = VEC3::Zero;
	VEC3 dir_entry_point = VEC3::Zero;
	VEC3 exit_point = VEC3::Zero;

	// Called when the associated button is activated
	void onEonInteracted(const TMsgEonInteracted& msg);

public:
	
	static void registerMsgs() {
		DECL_MSG(TCompEnergyWall, TMsgEonInteracted, onEonInteracted);
	}

	void update(float dt);
	bool isActive() { return is_active; }
	void reset() { eon_passed = false; };
	VEC3 calculateEntryPoint();
};
