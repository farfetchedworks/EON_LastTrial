#pragma once

#include "components/common/comp_base.h"
#include "entity/entity.h"
#include "components/messages.h"
#include "modules/module_physics.h"

class TCompBasicMechanism : public TCompBase {

	DECL_SIBLING_ACCESS();

private:

	float speed_multiplier			= 1.f;			// Speed multiplier to change time scale on Area Delay
	float speed						= 10.f;			// Movement speed of the mechanism (it doesn't rotate)
	float lerp_factor				= 0.f;
	float distance_positions		= 0.f;			// Distance between start and end positions, calculated on entity created
	
	bool is_in_target = true;						// set to true when the mechanism reached the target or the interpolator is >1
	
	VEC3 start_position;							// Start position obtained from the json
	VEC3 end_position;								// End position obtained from the json
	VEC3 current_target;							// Current target of the movement
	VEC3 current_start;								// Current start position from the movement
	VEC3 button_position = VEC3::Zero;				// position where the button is going to be spawned

	std::string other_target_name = "";				// name of other target (if applicable) to be activated by the button

	physx::PxU32 hit_mask;

	const interpolators::IInterpolator* interpolator = nullptr;

	CHandle mechanism_transform;
	CHandle mechanism_collider;
	CHandle spawned_button;

	void move(float dt);
	void moveMechanismToTarget(float dt);

	// Spawn the button in the button position
	void spawnButton();

	// Execute physics initialization
	void initializeCollider();

	// Called when the area delay affects the mechanism	
	void onApplyAreaDelay(const TMsgApplyAreaDelay& msg);
	void onRemoveAreaDelay(const TMsgRemoveAreaDelay& msg);
	
	// Called when the associated button is activated and deactivated
	void onButtonActivated(const TMsgButtonActivated& msg);
	void onButtonDeactivated(const TMsgButtonDeactivated& msg);

public:

	static void registerMsgs() {
		DECL_MSG(TCompBasicMechanism, TMsgApplyAreaDelay, onApplyAreaDelay);
		DECL_MSG(TCompBasicMechanism, TMsgRemoveAreaDelay, onRemoveAreaDelay);
		DECL_MSG(TCompBasicMechanism, TMsgButtonActivated, onButtonActivated);
		DECL_MSG(TCompBasicMechanism, TMsgButtonDeactivated, onButtonDeactivated);
	}

	CHandle getSpawnedButton() {
		return spawned_button;
	}

	void onEntityCreated();
	void update(float dt);
	void debugInMenu();
	void renderDebug();
	void load(const json& j, TEntityParseContext& ctx);
};
