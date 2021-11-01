#include "mcv_platform.h"
#include "handle/handle.h"
#include "engine.h"
#include "components/gameplay/comp_basic_mechanism.h"
#include "components/common/comp_transform.h"
#include "entity/entity.h"
#include "modules/module_physics.h"
#include "components/common/comp_collider.h"
#include "components/messages.h"
#include "entity/entity_parser.h"
#include "components/gameplay/comp_on_off_button.h"
#include "components/common/comp_tags.h"

DECL_OBJ_MANAGER("basic_mechanism", TCompBasicMechanism)

void TCompBasicMechanism::load(const json& j, TEntityParseContext& ctx)
{
	speed = j.value("speed", speed);
	if (j.is_object()) {
		end_position = loadVEC3(j, "end_position");
		button_position = loadVEC3(j, "button_position");
		other_target_name = j.value("other_target_name", other_target_name);
	}
	else {
		end_position = VEC3::Zero;
		button_position = VEC3::Zero;
		other_target_name = "";
	}
}

void TCompBasicMechanism::debugInMenu()
{
	ImGui::DragFloat("Speed", &speed, 5.f, 1.f, 100.f);
}

void TCompBasicMechanism::onEntityCreated()
{
	mechanism_transform = get<TCompTransform>();
	mechanism_collider = get<TCompCollider>();
	TCompTransform* c_transform = mechanism_transform;
	
	// Start where the mechanism is positioned
	start_position = c_transform->getPosition();
	
	interpolator = &interpolators::quartInInterpolator;

	// Initialize physics for the mechanism
	initializeCollider();
	spawnButton();
}

void TCompBasicMechanism::initializeCollider()
{
	// The entity containing the platform must be a rigid body. Gravity must be disabled on the rigid body, or the mechanism will fall
	TCompCollider* c_collider = mechanism_collider;
	physx::PxRigidDynamic* actor_collider = static_cast<physx::PxRigidDynamic*>(c_collider->actor);
	physx::PxRigidActor* rigid_actor_collider = static_cast<physx::PxRigidActor*>(c_collider->actor);
	actor_collider->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, true);

	// VERY important, so that there is no impulse between the player and the mechanism
	actor_collider->setMass(0.f);
	actor_collider->setMassSpaceInertiaTensor(physx::PxVec3(0.f));

	// To change filters dynamically, disable simulation, change the filtering and then enable it again
	EnginePhysics.setSimulationDisabled(actor_collider, true);
	actor_collider->setRigidDynamicLockFlags(physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_X | physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y | physx::PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z);

	hit_mask = CModulePhysics::FilterGroup::Characters | CModulePhysics::FilterGroup::Projectile | CModulePhysics::FilterGroup::Interactable | CModulePhysics::FilterGroup::EffectArea;
	EnginePhysics.setupFilteringOnAllShapesOfActor(c_collider->actor, CModulePhysics::FilterGroup::Door, hit_mask);
	EnginePhysics.setSimulationDisabled(actor_collider, false);
}

void TCompBasicMechanism::update(float dt)
{
	dt = dt * speed_multiplier;			// Apply the spped multiplier to the dt if the object is affected by area delay	
	moveMechanismToTarget(dt);
}

// Move the mechanism to the current target, either opening or closing
void TCompBasicMechanism::moveMechanismToTarget(float dt)
{
	if (!is_in_target) {										// Check if the mechanism has not arrived to its current target and move the mechanism
		move(dt);
	}
}

// Move the mechanism to the next position
void TCompBasicMechanism::move(float dt)
{
	TCompTransform* c_transform = mechanism_transform;
	CTransform destination_trans;
	VEC3 current_position = c_transform->getPosition();

	if (lerp_factor < 1.f) {
		
		// If t=distance/speed, the lerp_factor gets to 1, therefore for dt, interp = dt / (dist/speed)
		lerp_factor += dt / (distance_positions / speed);

		float f = lerp_factor;

		if (interpolator)
			f = interpolator->blend(0.f, 1.f, lerp_factor);

		VEC3 next_position = VEC3::Lerp(current_start, current_target, f);
		destination_trans.setPosition(next_position);
		destination_trans.setRotation(c_transform->getRotation());

		physx::PxTransform newTransform = toPxTransform(destination_trans);
		// Call setGlobalPose to move rigid bodies objects
		TCompCollider* c_collider = mechanism_collider;
		((physx::PxRigidDynamic*)c_collider->actor)->setGlobalPose(newTransform);
	}
	else {
		lerp_factor = 0.f;
		is_in_target = true;
	}

}

void TCompBasicMechanism::spawnButton()
{
	// Only spawn the button if a position was provided
	if (button_position != VEC3::Zero) {
		CTransform button_transform;
		button_transform.setPosition(button_position);
		spawned_button = spawn("data/prefabs/on_off_button.json", button_transform);
		CEntity* e_button = spawned_button;

		// Set the mechanism as the target of the trigger
		TCompOnOffButton* c_trigger_button = e_button->get<TCompOnOffButton>();

		CHandle other_target;
		if (other_target_name != "") {
			other_target = getEntityByName(other_target_name);
		}
		else {
			other_target = CHandle();
		}

		// Set the button to reset state after death
		TCompTags* c_mechanism_tags = get<TCompTags>();
		//bool has_to_be_reset = c_mechanism_tags->hasTag(getID("reset_button_state"));
		bool has_to_be_reset = true;
		
		// Inform the trigger button that it has to activate this object with priority over the second one
		c_trigger_button->setParameters(CHandle(this).getOwner(), other_target, has_to_be_reset);
	}
}

/*
	Messages callbacks
*/

void TCompBasicMechanism::onApplyAreaDelay(const TMsgApplyAreaDelay& msg)
{
	speed_multiplier = msg.speedMultiplier;
}

void TCompBasicMechanism::onRemoveAreaDelay(const TMsgRemoveAreaDelay& msg)
{
	speed_multiplier = 1.f;
}

void TCompBasicMechanism::onButtonActivated(const TMsgButtonActivated& msg)
{
	is_in_target = false;															// Set is in target to false to start moving
	TCompTransform* c_transform = mechanism_transform;
	
	// Start where the mechanism is positioned
	current_start = c_transform->getPosition();				// To consider cases when the mechanism was closing and was activated again
	current_target = end_position;										// The mechanism has to end at the original end position
	distance_positions = VEC3::Distance(current_start, current_target);
}

void TCompBasicMechanism::onButtonDeactivated(const TMsgButtonDeactivated& msg)
{
	is_in_target = false;															// Set is in target to false to start moving
	TCompTransform* c_transform = mechanism_transform;

	// Start where the mechanism is positioned
	current_start = c_transform->getPosition();				// To consider cases when the mechanism was opening and was deactivated again
	current_target = start_position;										// The mechanism has to end at the original end position
	distance_positions = VEC3::Distance(current_start, current_target);
}

void TCompBasicMechanism::renderDebug()
{


}
