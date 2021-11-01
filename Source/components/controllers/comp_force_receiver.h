#pragma once

#include "components/common/comp_base.h"
#include "entity/entity.h"
#include "components/messages.h"

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

class TCompForceReceiver : public TCompBase
{
	DECL_SIBLING_ACCESS();

private:
	// Forces and impulses
	VEC3 applied_force = VEC3::Zero;		// Used to apply forces if necessary on update
	float force_timer = 0.f;						// To control the lerp between the force and the null vector

	// Apply a force if there is one
	void applyForce(float dt);

	// Cached handles common to all pawns
	CHandle h_collider;
	CHandle h_transform;

public:

	void load(const json& j, TEntityParseContext& ctx);
	void renderDebug();
	
	void onEntityCreated();

	void movePhysics(VEC3 move_dir, float dt, float gravity = -9.81f);
	
	void addForce(VEC3 force);			// Add a force vector (pending to check if it is an impulse or a force)

	void onAddForce(const TMsgAddForce& msg);
	void onRemoveForces(const TMsgRemoveForces& msg);				// Removes all forces applied to the pawn

	void update(float dt);

	// Messages subscription
	static void registerMsgs() {
		DECL_MSG(TCompForceReceiver, TMsgAddForce, onAddForce);
		DECL_MSG(TCompForceReceiver, TMsgRemoveForces, onRemoveForces);
	}
};