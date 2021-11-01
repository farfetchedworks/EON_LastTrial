#include "mcv_platform.h"
#include "components/common/comp_base.h"
#include "components/common/comp_transform.h"
#include "components/common/comp_collider.h"
#include "engine.h"
#include "modules/module_physics.h"
#include "entity/entity.h"
#include "entity/entity_parser.h"

struct TCompWASDController : public TCompBase {

  DECL_SIBLING_ACCESS();

  CHandle h_my_transform;
  float m_speed = 15.0f;
  float r_speed = 6.0f;
	bool  enabled = false;

  void onEntityCreated() {
    h_my_transform = get<TCompTransform>();
  }

  void load(const json& j, TEntityParseContext& ctx) {
      m_speed = j.value("speed", m_speed);
      r_speed = j.value("rot_speed", r_speed);
      enabled = j.value("enabled", enabled);
  }

  void debugInMenu() {
    	ImGui::DragFloat("Move speed", &m_speed, 0.02f, 0.1f, 15.0f);
    	ImGui::DragFloat("Rotation speed", &r_speed, 0.02f, 0.1f, 15.0f);
		ImGui::Checkbox("Enabled", &enabled);
  }

  CTimer lastFireTimer;
  CTimer lastMeleeTimer;

  void fire() {
	  if (lastFireTimer.elapsed() > 0.5f)
	  {
		  lastFireTimer.reset();
		  TEntityParseContext ctx;
		  TCompTransform* c_trans = get<TCompTransform>();
		  ctx.root_transform = *c_trans;
		  ctx.root_transform.setPosition(ctx.root_transform.getPosition() + c_trans->getForward() * 2.f + c_trans->getUp() * 2.f);
		  parseScene("data/prefabs/bullet.json", ctx);

		  CEntity * e = ctx.entities_loaded[0];
		  TCompCollider * compCollider = e->get<TCompCollider>();
		  if (compCollider != nullptr)
		  {
			  VEC3 force = c_trans->getForward() + VEC3(0.f, 0.5f, 0.f);
			  force *= 10.f;
			  if (compCollider->actor != nullptr)
			  {
				  ((physx::PxRigidDynamic*)compCollider->actor)->addForce(VEC3_TO_PXVEC3(force), physx::PxForceMode::eIMPULSE);
			  }
		  }
		  VEC3 origin = ctx.root_transform.getPosition(); +c_trans->getForward() * 0.1f + c_trans->getUp() * 0.1f;
		  physx::PxRaycastBufferN<10> hitCall;
		  physx::PxQueryFilterData fd;
		  fd.flags |= physx::PxQueryFlag::eANY_HIT; // note the OR with the default value
		  bool status = EnginePhysics.gScene->raycast(VEC3_TO_PXVEC3(origin), VEC3_TO_PXVEC3(c_trans->getForward()), 200.f, hitCall, physx::PxHitFlags(physx::PxHitFlag::eDEFAULT), fd);

		  if (status)
		  {
			  if (hitCall.hasAnyHits())
			  {
				  unsigned int numHits = hitCall.getNbAnyHits();
				  for (unsigned int i = 0; i < hitCall.getNbAnyHits(); ++i)
				  {
					  const physx::PxRaycastHit& overlapHit = hitCall.getAnyHit(i);

					  CHandle h_collider;
					  h_collider.fromVoidPtr(overlapHit.actor->userData);
					  TCompCollider* c_collider = h_collider;
					  if (c_collider != nullptr)
					  {
						  CEntity* e = CHandle(c_collider).getOwner();
						  dbg("fire! raycast hit: %s \n", e->getName());
					  }
				  }
			  }
		  }
	  }
  }

  void melee() {
	  if (lastMeleeTimer.elapsed() > 0.5f)
	  {
		  lastMeleeTimer.reset();
		  TCompTransform* c_trans = get<TCompTransform>();
		  VEC3 origin = c_trans->getPosition();
		  physx::PxQueryFilterData fd;
		  //fd.flags |= physx::PxQueryFlag::eANY_HIT; // note the OR with the default value
		  physx::PxOverlapBufferN<10> hit;
		  physx::PxGeometry overlapShape = physx::PxSphereGeometry(100.f);  // [in] shape to test for overlaps
		  physx::PxTransform shapePose(VEC3_TO_PXVEC3(origin));

		  bool status = EnginePhysics.gScene->overlap(overlapShape, shapePose, hit, fd);

		  if (status)
		  {
			  if (hit.hasAnyHits())
			  {
				  CEntity* owner = CHandle(this).getOwner();

				  unsigned int numHits = hit.getNbAnyHits();
				  for (unsigned int i = 0; i < hit.getNbAnyHits(); ++i)
				  {
					const physx::PxOverlapHit& overlapHit = hit.getAnyHit(i);

					CHandle h_collider;
					h_collider.fromVoidPtr(overlapHit.actor->userData);
					TCompCollider* c_collider = h_collider;
					if (c_collider != nullptr)
					{
						  CEntity* e = CHandle(c_collider).getOwner();
						  dbg("melee! overlap hit: %s \n", e->getName());
						  if (owner != e) //ignore myself
						  {
							  VEC3 force = c_trans->getForward() + VEC3(0.f, 0.f, 0.f);
							  if (c_collider->actor != nullptr)
							  {
								  ((physx::PxRigidDynamic*)c_collider->actor)->addForce(VEC3_TO_PXVEC3(force), physx::PxForceMode::eIMPULSE);
							  }
						  }
					}
				  }
			  }
		  }
	  }
  }

  void update(float dt) {

		if (!enabled)
			return;

    // Basic player movement

    TCompTransform* t = h_my_transform;

	VEC3 new_move;
    if (isPressed('W'))
		new_move = t->getForward() * m_speed * dt;
    if (isPressed('S'))
		new_move = -t->getForward() * m_speed * dt;

	TCompCollider* compCollider = get<TCompCollider>();
	if (compCollider != nullptr && compCollider->cap_controller)
	{
		new_move.y += -9.81f * dt;

		static const physx::PxU32 max_shapes = 8;
		physx::PxShape* shapes[max_shapes];

		physx::PxU32 nshapes = compCollider->cap_controller->getActor()->getNbShapes();
		assert(nshapes <= max_shapes);

		// Even when the buffer is small, it writes all the shape pointers
		physx::PxU32 shapes_read = compCollider->cap_controller->getActor()->getShapes(shapes, sizeof(shapes), 0);

		// Make a copy of the pxFilterData because the result of getQueryFilterData is returned by copy
		physx::PxFilterData filterData = shapes[0]->getQueryFilterData();
		physx::PxControllerFilters controllerFilters(&filterData, &EnginePhysics.customQueryFilterCallback);
		compCollider->cap_controller->move(physx::PxVec3(new_move.x, new_move.y, new_move.z), 0.0f, dt, controllerFilters);
	}
	else
	{
		t->setPosition(t->getPosition() + new_move);
	}

    if (isPressed('A')) {
      QUAT delta_rotation = QUAT::CreateFromAxisAngle(VEC3(0, 1, 0), r_speed * dt);
      t->setRotation(t->getRotation() * delta_rotation);
    }
    if (isPressed('D')) {
      QUAT delta_rotation = QUAT::CreateFromAxisAngle(VEC3(0, 1, 0), -r_speed * dt);
      t->setRotation(t->getRotation() * delta_rotation);
    }

	if (isPressed(' '))
	{
		fire();
	}
	if (isPressed('V'))
	{
		melee();
	}
  }

};

DECL_OBJ_MANAGER("wasd_controller", TCompWASDController)