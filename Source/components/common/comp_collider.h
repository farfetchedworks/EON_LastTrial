#pragma once

#include "entity/entity.h"
#include "comp_base.h"
#include "PxPhysicsAPI.h"

#define VEC3_TO_PXVEC3(vec3) physx::PxVec3(vec3.x, vec3.y, vec3.z)
#define VEC3_TO_PXEXVEC3(vec3) physx::PxExtendedVec3(vec3.x, vec3.y, vec3.z)
#define PXVEC3_TO_VEC3(pxvec3) VEC3(pxvec3.x, pxvec3.y, pxvec3.z)
#define PXEXVEC3_TO_VEC3(pxexvec3) VEC3(static_cast<float>(pxexvec3.x), static_cast<float>(pxexvec3.y), static_cast<float>(pxexvec3.z))

#define QUAT_TO_PXQUAT(quat) physx::PxQuat(quat.x, quat.y, quat.z, quat.w)
#define PXQUAT_TO_QUAT(pxquat) QUAT(pxquat.x, pxquat.y, pxquat.z,pxquat.w)

class TCompCollider : public TCompBase {
private:
	CHandle h_transform;

	void createController(bool isCapsuleController);
	void activateForceActor(bool disableGravity = false);
	void followForceActor();

public:
	DECL_SIBLING_ACCESS();

	bool is_controller = false;
	bool is_dynamic = false;
	bool simulation_disabled = false;
	bool active_force = false;
	bool disable_gravity = true;
	bool is_capsule_controller = true;
	//bool disabled_box_controller = true;

	std::string shape;
	std::string mask;
	float radius = 0.f;
	float height = 0.f;
	float min_vel = 15.f;
	int forces_num = 0;
	std::map<std::string, VEC3> applied_forces;						// Stores all the applied forces when one of them needs to be removed

	json jconfig;
	physx::PxBoxController* box_controller = nullptr;
	physx::PxCapsuleController* cap_controller = nullptr;
	physx::PxRigidActor* actor = nullptr;
	physx::PxRigidActor* c_actor = nullptr;
	physx::PxRigidActor* b_actor = nullptr;
	physx::PxRigidActor* force_actor = nullptr;
	physx::PxRigidBody* rigidbody = nullptr;
	physx::PxF32 _stepOffset = 0.f;

	void setSphereShapeRadius(float radius);

	void setGlobalPose(VEC3 new_pos, QUAT new_rotation, bool autowake = true);
	void addForce(VEC3 force, const std::string force_origin, bool disableGravity = false, float minVel = 0.2f);
	void stopFollowForceActor(const std::string force_origin);
	bool collisionAtDistance(const VEC3& org, const VEC3& dir, float maxDistance, float& distance);
	float distanceToGround();
	VEC3 getLinearVelocity();

	~TCompCollider();
	void releaseController();
	void load(const json& j, TEntityParseContext& ctx);
	void onEntityCreated();
	void createForceActor();
	void update(float dt);
	void debugInMenu();
	void renderDebug();

	void setTriggerAs(bool);
	void debugInMenuShape(physx::PxShape* shape, physx::PxGeometryType::Enum geometry_type, const void* geom, MAT44 world);
	void renderShape(physx::PxShape* shape, physx::PxGeometryType::Enum geometry_type, const void* geom, MAT44 world);
	void setGroupAndMask(const std::string& group, const std::string& mask);
	void disable(bool disabled);

	typedef void (TCompCollider::* TShapeFn)(physx::PxShape* shape, physx::PxGeometryType::Enum geometry_type, const void* geom, MAT44 world);
	void onEachShape(TShapeFn fn);
	void setStepOffset(float stepOffset);
	void restoreStepOffset();
	void setControllerPosition(VEC3 pos);
	void setFootPosition(VEC3 pos);
	physx::PxExtendedVec3 getControllerPosition() { return is_capsule_controller ? cap_controller->getPosition() : box_controller->getPosition(); }
	physx::PxExtendedVec3 getFootPosition() { return is_capsule_controller ? cap_controller->getFootPosition() : box_controller->getFootPosition(); }
	physx::PxU32 getControllerNbShapes();
	void enableBoxController();
	void enableCapsuleController();
};
