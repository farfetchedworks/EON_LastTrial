#include "mcv_platform.h"
#include "comp_hierarchy.h"
#include "comp_transform.h"
#include "comp_parent.h"
#include "entity/entity.h"
#include "entity/entity_parser.h"
#include "comp_collider.h"
#include "modules/module_physics.h"

DECL_OBJ_MANAGER("hierarchy", TCompHierarchy)

void TCompHierarchy::debugInMenu() {
  CTransform::renderInMenu();
}

void TCompHierarchy::load(const json& j, TEntityParseContext& ctx) {
  CTransform::fromJson(j);
  
  assert(j.count("parent"));
  parent_name = j.value("parent", "");

  updates_transform = j.value("updates_transform", updates_transform);

  CHandle h_entity_parent = ctx.findEntityByName(parent_name);
  CEntity* e_entity_parent = h_entity_parent;
  assert(e_entity_parent);

  TCompParent* c_parent = e_entity_parent->get<TCompParent>();
  assert(c_parent);

  c_parent->addChild(CHandle(this).getOwner());

  h_parent_transform = e_entity_parent->get<TCompTransform>();
}

void TCompHierarchy::onEntityCreated() {
  assert(!h_my_transform.isValid());
  h_my_transform = get<TCompTransform>();
}

void TCompHierarchy::update(float dt) {
  
  if (!updates_transform) {
    return;
  }

  // The target transform
  TCompTransform* c_parent_transform = h_parent_transform;
  if (!c_parent_transform)
    return;

  TCompTransform* c_my_transform = h_my_transform;
  assert(c_my_transform || fatal( "Entity %s comp hierarchy is missing the TCompTransform", getEntity() ? getEntity()->getName() : "Unknown" ) );

  // Apply Delta(me) to the parent to set the transform of my entity
  c_my_transform->set(c_parent_transform->combinedWith(*this));


  // Updates the physics actor if it has one
  TCompCollider* c_self_collider = get<TCompCollider>();
  if (c_self_collider) {
    //VEC3 current_pos = c_my_transform->getPosition();
    physx::PxRigidDynamic* rigid_actor = static_cast<physx::PxRigidDynamic *>(c_self_collider->actor);
    
    physx::PxTransform new_pose = toPxTransform(*c_my_transform);
    
    
    bool isKinematic = rigid_actor->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC;

    // If the actor is a rigid dynamic, set global pose
    if (!isKinematic) {
      c_self_collider->actor->setGlobalPose(new_pose);   
    }
    else {  // If the actor is kinematic, set kinematic target
      ((physx::PxRigidDynamic*)c_self_collider->actor)->setKinematicTarget(new_pose);
    }
  }

}

CEntity* TCompHierarchy::getRootEntity(CEntity* entity)
{
    TCompHierarchy* c_hierarchy = entity->get<TCompHierarchy>();
    if (c_hierarchy) {
        CEntity* parent_entity = getEntityByName(c_hierarchy->parent_name);
        return getRootEntity(parent_entity);
    }

    return entity;
}
