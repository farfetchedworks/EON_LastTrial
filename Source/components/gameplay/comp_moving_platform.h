#pragma once

#include "components/common/comp_base.h"
#include "entity/entity.h"
#include "components/messages.h"

class TCompMovingPlatform : public TCompBase {

	DECL_SIBLING_ACCESS();

private:

	float current_time = 0.f;									
	float speed_multiplier = 1.f;							// Speed multiplier to change time scale on Area Delay
	float speed = 10.f;												// Movement speed of the projectile (it doesn't rotate)
	float interpolator = 0.f;
	float distance_positions = 0.f;						// Distance between start and end positions, calculated on entity created
	
	VEC3 start_position;
	VEC3 end_position;
	VEC3 current_target;

	CHandle platform_transform;
	CHandle platform_collider;

	void move(float dt);

	// Called when the projectile has hit anything	
	void onApplyAreaDelay(const TMsgApplyAreaDelay& msg);
	void onRemoveAreaDelay(const TMsgRemoveAreaDelay& msg);

public:

	static void registerMsgs() {
		DECL_MSG(TCompMovingPlatform, TMsgApplyAreaDelay, onApplyAreaDelay);
		DECL_MSG(TCompMovingPlatform, TMsgRemoveAreaDelay, onRemoveAreaDelay);
	}

	void onEntityCreated();
	void update(float dt);
	void debugInMenu();
	void renderDebug();
	void load(const json& j, TEntityParseContext& ctx);
};
