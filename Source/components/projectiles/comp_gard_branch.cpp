#include "mcv_platform.h"
#include "components/projectiles/comp_gard_branch.h"
#include "components/common/comp_transform.h"
#include "engine.h"
#include "modules/module_physics.h"
#include "components/combat/comp_hitmap.h"
#include "skeleton/comp_skeleton.h"

DECL_OBJ_MANAGER("gard_branch", TCompGardBranch)

void TCompGardBranch::load(const json& j, TEntityParseContext& ctx)
{
	damage = j.value("damage", damage);
}

void TCompGardBranch::setParameters(int new_damage)
{
	damage = new_damage;
}

void TCompGardBranch::onEntityCreated()
{	
	h_branch_trans = get<TCompTransform>();
	TCompTransform* c_trans = h_branch_trans;

	// Gravity must be disabled on the rigid body, or the branch will fall
	h_branch_collider = get<TCompCollider>();
	TCompCollider* c_collider = h_branch_collider;
	physx::PxRigidDynamic* actor_collider = static_cast<physx::PxRigidDynamic*>(c_collider->actor);
	actor_collider->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, true);

	TCompSkeleton* skel = get<TCompSkeleton>();
	skel->playAction("Spawn", 0.f, 0.f);
}

void TCompGardBranch::destroy()
{
	// Destroy branch
	CHandle(this).getOwner().destroy();
}

void TCompGardBranch::update(float dt)
{
	
}

// When the branch hits something, sends a message to the player to damage it
void TCompGardBranch::onHitObject(const TMsgEntityOnContact& msg)
{
	CEntity* e_object_hit = msg.h_entity;
	if (!e_object_hit)
		return;

	// Check hit map
	{
		TCompHitmap* hitmap = get<TCompHitmap>();
		if (hitmap) {
			if (!hitmap->resolve(msg.h_entity))
				return;
		}
	}

	TCompTransform* c_branch_trans = get<TCompTransform>();

	// Damage the player
	TMsgHit msgHit;
	msgHit.h_striker = CHandle(this).getOwner();
	msgHit.position = c_branch_trans->getPosition();
	msgHit.forward = c_branch_trans->getForward();
	msgHit.damage = damage;
	msgHit.hitType = EHIT_TYPE::GARD_BRANCH;
	e_object_hit->sendMsg(msgHit);

	// Apply a pushback force
	TCompTransform* c_player_trans = e_object_hit->get<TCompTransform>();
	VEC3 force_dir = c_player_trans->getPosition() - c_branch_trans->getPosition();
	force_dir.y = 0.f;
	float strength = 10.f;
	force_dir.Normalize();

	TMsgAddForce msgForce;
	msgForce.force = force_dir * strength;
	msgForce.h_applier = msgHit.h_striker;
	e_object_hit->sendMsg(msgForce);

}

void TCompGardBranch::debugInMenu()
{
	ImGui::DragInt("Damage", &damage, 5, 1, 100);
}