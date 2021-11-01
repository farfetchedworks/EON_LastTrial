#pragma once

#include "components/common/comp_base.h"
#include "entity/entity.h"
#include "components/messages.h"
#include "modules/module_physics.h"

class TCompTimedDoor : public TCompBase {

	DECL_SIBLING_ACCESS();

private:

	float current_time			= 0.f;									
	float speed_multiplier		= 1.f;		// Speed multiplier to change time scale on Area Delay
	float speed					= 12.f;		// Movement speed of the door (it doesn't rotate)
	float lerp_factor			= 0.f;
	float distance_positions	= 0.f;		// Distance between start and end positions, calculated on entity created
	float closing_time			= 2.f;		// Time before the door starts closing
	
	bool is_activated			= false;	// set to true once the door is opening
	bool is_completely_open		= false;	// set to true when the door is completely open
	bool is_in_target			= false;	// set to true when the door reached the target or the interpolator is >1
	
	VEC3 start_position;
	VEC3 end_position;
	VEC3 button_position;					// position where the button is going to be spawned

	physx::PxU32 hit_mask;

	CHandle door_transform;
	CHandle door_collider;

	const interpolators::IInterpolator* interpolator = nullptr;

	void move(float dt);
	void openDoor(float dt);
	void closeDoor(float dt);

	// Spawn the button in the button position
	void spawnButton();

	// Execute physics initialization
	void initializeCollider();

	// Called when the area delay affects the door	
	void onApplyAreaDelay(const TMsgApplyAreaDelay& msg);
	void onRemoveAreaDelay(const TMsgRemoveAreaDelay& msg);
	
	// Called when the associated button is activated
	void onButtonActivated(const TMsgButtonActivated& msg);

public:

	static void registerMsgs() {
		DECL_MSG(TCompTimedDoor, TMsgApplyAreaDelay, onApplyAreaDelay);
		DECL_MSG(TCompTimedDoor, TMsgRemoveAreaDelay, onRemoveAreaDelay);
		DECL_MSG(TCompTimedDoor, TMsgButtonActivated, onButtonActivated);
	}

	void onEntityCreated();
	void update(float dt);
	void debugInMenu();
	void renderDebug();
	void load(const json& j, TEntityParseContext& ctx);
};
