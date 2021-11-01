#include "mcv_platform.h"
#include "handle/handle.h"
#include "engine.h"
#include "components/gameplay/comp_moving_platform.h"
#include "components/common/comp_transform.h"
#include "entity/entity.h"
#include "modules/module_physics.h"
#include "components/common/comp_collider.h"
#include "components/messages.h"

DECL_OBJ_MANAGER("moving_platform", TCompMovingPlatform)

void TCompMovingPlatform::load(const json& j, TEntityParseContext& ctx)
{
	speed = j.value("speed", speed);
	if (j.is_object()) {
		end_position = loadVEC3(j, "end_position");
	}
	else {
		end_position = VEC3::Zero;
	}
}

void TCompMovingPlatform::debugInMenu()
{
	ImGui::DragFloat("Speed", &speed, 5.f, 1.f, 100.f);
}

void TCompMovingPlatform::onEntityCreated()
{
	platform_transform = get<TCompTransform>();
	platform_collider = get<TCompCollider>();
	TCompTransform* c_transform = platform_transform;
	
	// Start where the platform is positioned
	start_position = c_transform->getPosition();
	//current_target = end_position;
	distance_positions = VEC3::Distance(start_position, end_position);
		
	// The entity containing the platform must be a rigid body. Gravity must be disabled on the rigid body, or the projectile will fall
	TCompCollider* c_collider = platform_collider;
	physx::PxRigidDynamic* actor_collider = static_cast<physx::PxRigidDynamic*>(c_collider->actor);
	actor_collider->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, true);
	actor_collider->setRigidDynamicLockFlags(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_X| physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y| physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z);
}

void TCompMovingPlatform::update(float dt)
{
	dt = dt * speed_multiplier;			// Apply the spped multiplier to the dt if the object is affected by area delay
	move(dt);

}

// Move the platform to the next position
void TCompMovingPlatform::move(float dt)
{
	TCompTransform* c_transform = platform_transform;
	CTransform destination_trans;
	VEC3 current_position = c_transform->getPosition();

	if (current_position != end_position && interpolator <= 1.f) {
		
		VEC3 next_position = VEC3::Lerp(start_position, end_position, interpolator);

		// If t=distance/speed, the interpolator gets to 1, therefore for dt, interp = dt / (dist/speed)
		interpolator += dt / (distance_positions / speed);
		destination_trans.setPosition(next_position);
		destination_trans.setRotation(c_transform->getRotation());

		physx::PxTransform newTransform = toPxTransform(destination_trans);
		// Call setGlobalPose to move rigid bodies objects
		TCompCollider* c_collider = platform_collider;
		((physx::PxRigidDynamic*)c_collider->actor)->setGlobalPose(newTransform);
	}
	else {
		std::swap<VEC3>(start_position, end_position);
		interpolator = 0.f;
	}

}

void TCompMovingPlatform::onApplyAreaDelay(const TMsgApplyAreaDelay& msg)
{
	speed_multiplier = msg.speedMultiplier;
}

void TCompMovingPlatform::onRemoveAreaDelay(const TMsgRemoveAreaDelay& msg)
{
	speed_multiplier = 1.f;
}

void TCompMovingPlatform::renderDebug()
{


}

