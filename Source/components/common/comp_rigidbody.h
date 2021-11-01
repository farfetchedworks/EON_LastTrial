#pragma once

#include "comp_base.h"
#include "PxPhysicsAPI.h"
#include "components/messages.h"

class TCompRigidbody : public TCompBase {

    physx::PxVec3 velocity = physx::PxVec3(0, 0, 0);

    DECL_SIBLING_ACCESS();

private:

    // Cached handlers
    CHandle h_collider;

    physx::PxRigidBody* rigidbody = nullptr;

public:


    float mass;
    float max_depenetration_velocity;
    float max_contact_impulse;
    float angular_drag;

    bool is_gravity;
    bool is_kinematic;
    bool is_controller;
    bool is_enabled;

    ~TCompRigidbody();
    void load(const json& j, TEntityParseContext& ctx);
    void onEntityCreated();
    void addForce(VEC3 force);
    float getMagnitude();
};