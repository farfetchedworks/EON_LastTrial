#include "mcv_platform.h"
#include "comp_rigidbody.h"
#include "comp_collider.h"
#include "comp_transform.h"
#include "engine.h"
#include "modules/module_physics.h"

DECL_OBJ_MANAGER("rigid_body", TCompRigidbody);

TCompRigidbody::~TCompRigidbody() {
   
}


void TCompRigidbody::load(const json& j, TEntityParseContext& ctx) {
    
	assert(j.is_object());

    mass = j.value("mass", 1.f);
	max_depenetration_velocity = j.value("max_depenetration_velocity", 1.f);
	max_contact_impulse = j.value("max_contact_impulse", 1.f);

    is_enabled = true;
    is_gravity = j.value("is_gravity", false);
    is_kinematic = j.value("is_kinematic", false);
}

void TCompRigidbody::onEntityCreated()
{
	h_collider = get<TCompCollider>();
}

void TCompRigidbody::addForce(VEC3 force)
{
	TCompCollider* c_collider = h_collider;
	rigidbody = ((physx::PxRigidBody*)c_collider->actor);

	if (rigidbody != nullptr) {

		rigidbody->addForce(VEC3_TO_PXVEC3(force), physx::PxForceMode::eIMPULSE);
	}	
}

float TCompRigidbody::getMagnitude()
{
	TCompCollider* c_collider = h_collider;
	rigidbody = ((physx::PxRigidBody*)c_collider->actor);

	return rigidbody->getLinearVelocity().magnitude();
}