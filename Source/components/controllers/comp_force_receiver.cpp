#include "mcv_platform.h"
#include "comp_force_receiver.h"
#include "engine.h"
#include "entity/entity.h"
#include "components/common/comp_collider.h"
#include "components/common/comp_transform.h"
#include "components/controllers/pawn_utils.h"
#include "entity/entity_parser.h"

DECL_OBJ_MANAGER("force_receiver", TCompForceReceiver)

void TCompForceReceiver::load(const json& j, TEntityParseContext& ctx)
{

}

void TCompForceReceiver::onEntityCreated()
{
	h_transform = get<TCompTransform>();
	h_collider = get<TCompCollider>();
}

void TCompForceReceiver::update(float dt)
{
	applyForce(dt);
}

void TCompForceReceiver::movePhysics(VEC3 move_dir, float dt, float gravity)
{
	if (h_collider.isValid()) {
		PawnUtils::movePhysics(h_collider, move_dir, dt, gravity);
	}
}

void TCompForceReceiver::addForce(VEC3 new_force)
{
	applied_force += new_force;
}

void TCompForceReceiver::applyForce(float dt)
{
	if (applied_force != VEC3::Zero) {
		force_timer += dt;
		
		VEC3 new_position;
		VEC3 force_direction;
		float lerp_value = force_timer;		// multiplied by a factor to generate the impulse

		applied_force.Normalize(force_direction);
		applied_force = lerp<VEC3>(applied_force, VEC3::Zero, lerp_value);
		float force = applied_force.Length();

		new_position = force_direction * force * dt;
		movePhysics(new_position, dt, -2.0);

		if (lerp_value >= 1.f || force < 0.05)
		{
			TMsgRemoveForces msg;
			h_collider.getOwner().sendMsg(msg);
			force_timer = 0.f;
			applied_force = {};
		}
	}
}


// Message callbacks

void TCompForceReceiver::onRemoveForces(const TMsgRemoveForces& msg)
{
	if (msg.byPlayer) {
		applied_force = VEC3::Zero;
		force_timer = 0.f;
	}
	else {
		static_cast<TCompCollider*>(h_collider)->stopFollowForceActor(msg.force_origin);
	}
}

void TCompForceReceiver::onAddForce(const TMsgAddForce& msg)
{
	msg.byPlayer ? addForce(msg.force) : static_cast<TCompCollider*>(h_collider)->addForce(msg.force, msg.force_origin, msg.disableGravity);
}


void TCompForceReceiver::renderDebug()
{
#ifdef _DEBUG

#endif
}