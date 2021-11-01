#include "mcv_platform.h"
#include "handle/handle.h"
#include "engine.h"
#include "components/gameplay/comp_timed_door.h"
#include "components/common/comp_transform.h"
#include "entity/entity.h"
#include "modules/module_physics.h"
#include "components/common/comp_collider.h"
#include "components/messages.h"
#include "entity/entity_parser.h"
#include "components/gameplay/comp_trigger_button.h"

DECL_OBJ_MANAGER("timed_door", TCompTimedDoor)

void TCompTimedDoor::load(const json& j, TEntityParseContext& ctx)
{
	speed = j.value("speed", speed);
	closing_time = j.value("closing_time", closing_time);
	if (j.is_object()) {
		end_position = loadVEC3(j, "end_position");
	}
	else {
		end_position = VEC3::Zero;
	}
	if (j.is_object()) {
		button_position = loadVEC3(j, "button_position");
	}
	else {
		button_position = VEC3::Zero;
	}
}

void TCompTimedDoor::debugInMenu()
{
	ImGui::DragFloat("Speed", &speed, 1.f, 1.f, 100.f);
	ImGui::DragFloat("Closing time", &closing_time, 1.f, 1.f, 100.f);
}

void TCompTimedDoor::onEntityCreated()
{
	door_transform = get<TCompTransform>();
	door_collider = get<TCompCollider>();
	TCompTransform* c_transform = door_transform;
	
	// Start where the platform is positioned
	start_position = c_transform->getPosition();
	distance_positions = VEC3::Distance(start_position, end_position);
	
	interpolator = &interpolators::quartInInterpolator;

	initializeCollider();
	spawnButton();
}

void TCompTimedDoor::initializeCollider()
{
	// The entity containing the platform must be a rigid body. Gravity must be disabled on the rigid body, or the projectile will fall
	TCompCollider* c_collider = door_collider;
	physx::PxRigidDynamic* actor_collider = static_cast<physx::PxRigidDynamic*>(c_collider->actor);
	physx::PxRigidActor* rigid_actor_collider = static_cast<physx::PxRigidActor*>(c_collider->actor);
	actor_collider->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, true);

	// VERY important, so that there is no impulse between the player and the door
	actor_collider->setMass(0.f);
	actor_collider->setMassSpaceInertiaTensor(physx::PxVec3(0.f));

	// To change filters dynamically, disable simulation, change the filtering and then enable it again
	EnginePhysics.setSimulationDisabled(actor_collider, true);
	actor_collider->setRigidDynamicLockFlags(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_X | physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y | physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z);

	hit_mask = CModulePhysics::FilterGroup::Player | CModulePhysics::FilterGroup::Projectile | CModulePhysics::FilterGroup::Interactable | CModulePhysics::FilterGroup::EffectArea;
	EnginePhysics.setupFilteringOnAllShapesOfActor(c_collider->actor, CModulePhysics::FilterGroup::Door, hit_mask);
	EnginePhysics.setSimulationDisabled(actor_collider, false);
}

void TCompTimedDoor::update(float dt)
{
	dt *= speed_multiplier;			// Apply the spped multiplier to the dt if the object is affected by area delay
	
	if (!is_activated)
		return;
	
	if (!is_completely_open) {
		openDoor(dt);
	}
	else {
		closeDoor(dt);
	}
}

// Move the platform to the next position
void TCompTimedDoor::move(float dt)
{
	TCompTransform* c_transform = door_transform;
	CTransform destination_trans;
	VEC3 current_position = c_transform->getPosition();

	if (lerp_factor < 1.f) {
		
		// If t=distance/speed, the interpolator gets to 1, therefore for dt, interp = dt / (dist/speed)
		lerp_factor += dt / (distance_positions / speed);

		float f = lerp_factor;

		if (interpolator)
			f = interpolator->blend(0.f, 1.f, lerp_factor);

		VEC3 next_position = VEC3::Lerp(start_position, end_position, f);

		destination_trans.setPosition(next_position);
		destination_trans.setRotation(c_transform->getRotation());

		physx::PxTransform newTransform = toPxTransform(destination_trans);
		// Call setGlobalPose to move rigid bodies objects
		TCompCollider* c_collider = door_collider;
		((physx::PxRigidDynamic*)c_collider->actor)->setGlobalPose(newTransform);
	}
	else {
		std::swap<VEC3>(start_position, end_position);
		lerp_factor = 0.f;
		is_in_target = true;
	}
}

void TCompTimedDoor::openDoor(float dt)
{
	if (!is_in_target) {						// Check if the door has not arrived to its current target and move the door
		move(dt);
	}
	else {
		is_completely_open = true;				// when the door reached its target, it has got to the end (it is open)
		is_in_target = false;					// reset is in target so that close door can move the door
	}
}

void TCompTimedDoor::closeDoor(float dt)
{
	current_time += dt;
	if (current_time >= closing_time) {			// Close the door after the closing time has elapsed

		if (!is_in_target) {					// Check if the door has not arrived to its current target and move the door
			move(dt);
		}
		else {
			current_time = 0.f;
			is_completely_open = false;			// when the door reached its target, it has got to the end (it is open)
			is_in_target = false;				// reset is in target so that open door can move the door
			is_activated = false;				// the door is now deactivated, waiting for a message from the button

			// Send a message to the button to change the color/mesh!
		}
	}
}

void TCompTimedDoor::spawnButton()
{
	CTransform button_transform;
	button_transform.setPosition(button_position);
	CEntity* e_button = spawn("data/prefabs/interactive_button.json", button_transform);

	// Set the door as the target of the trigger
	TCompTriggerButton* c_trigger_button = e_button->get<TCompTriggerButton>();
	c_trigger_button->setParameters(CHandle(this).getOwner());
}

/*
	Messages callbacks
*/

void TCompTimedDoor::onApplyAreaDelay(const TMsgApplyAreaDelay& msg)
{
	speed_multiplier = msg.speedMultiplier;
}

void TCompTimedDoor::onRemoveAreaDelay(const TMsgRemoveAreaDelay& msg)
{
	speed_multiplier = 1.f;
}

void TCompTimedDoor::onButtonActivated(const TMsgButtonActivated& msg)
{
	is_activated = true;
}

void TCompTimedDoor::renderDebug()
{


}

//// Move the platform to the next position
//void TCompTimedDoor::move(float dt)
//{
//	TCompTransform* c_transform = door_transform;
//	CTransform destination_trans;
//	VEC3 current_position = c_transform->getPosition();
//
//	if (current_position != end_position && interpolator <= 1.f) {
//
//		VEC3 next_position = VEC3::Lerp(start_position, end_position, interpolator);
//
//		// If t=distance/speed, the interpolator gets to 1, therefore for dt, interp = dt / (dist/speed)
//		interpolator += dt / (distance_positions / speed);
//		destination_trans.setPosition(next_position);
//		destination_trans.setRotation(c_transform->getRotation());
//
//		physx::PxTransform newTransform = toPxTransform(destination_trans);
//		// Call setGlobalPose to move rigid bodies objects
//		TCompCollider* c_collider = door_collider;
//		((physx::PxRigidDynamic*)c_collider->actor)->setGlobalPose(newTransform);
//	}
//	else {
//		std::swap<VEC3>(start_position, end_position);
//		interpolator = 0.f;
//	}
//
//}
