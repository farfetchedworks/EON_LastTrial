#include "mcv_platform.h"
#include "engine.h"
#include "entity/entity.h"
#include "entity/entity_parser.h"			
#include "pawn_utils.h"
#include "modules/module_physics.h"
#include "components/common/comp_transform.h"
#include "components/common/comp_collider.h"
#include "components/controllers/comp_player_controller.h"
#include "skeleton/comp_skeleton.h"
#include "skeleton/game_core_skeleton.h"
#include "components/stats/comp_geons_drop.h"
#include "components/particles/comp_follow_target.h"
#include "../bin/data/shaders/constants_particles.h"
#include "audio/module_audio.h"

bool CORRECT_MCV_AXIS = true;

bool PawnUtils::isInsideCone(CHandle h_me, CHandle h_other, float fov, float radius)
{
	assert(h_me.isValid() && h_other.isValid());
	if (!h_me.isValid() || !h_other.isValid())
	{
		dbg("isInsideCone with invalid CHandles\n");
		return false;
	}

	CEntity* me = h_me;
	CEntity* other = h_other;

	TCompTransform* my_trans = me->get<TCompTransform>();
	TCompTransform* other_trans = other->get<TCompTransform>();
	
	return my_trans->isInsideCone(other_trans->getPosition(), fov, radius);
}

bool PawnUtils::isInsideConeOfBone(CHandle h_me, CHandle h_other, const std::string& bone_name, float fov, float radius)
{
	assert(h_me.isValid() && h_other.isValid());
	if (!h_me.isValid() || !h_other.isValid())
	{
		dbg("isInsideConeOfBone with invalid CHandles\n");
		return false;
	}

	CEntity* me = h_me;
	CEntity* other = h_other;

	TCompSkeleton* c_skel = me->get<TCompSkeleton>();
	assert(c_skel);
	if (!c_skel)
		return false;

	CTransform my_bone_trans = c_skel->getWorldTransformOfBone(bone_name, CORRECT_MCV_AXIS);
	TCompTransform* other_trans = other->get<TCompTransform>();

	return my_bone_trans.isInsideCone(other_trans->getPosition(), fov, radius);
}

float PawnUtils::distToPlayer(CHandle h_me, bool squared)
{
	if (!h_me.isValid())
	{
		dbg("-- CHandle not valid in distToPlayer --\n");
		return 0;
	}

	CEntity* player = getEntityByName("player");
	CEntity* me = h_me;

	if(squared)
		return VEC3::DistanceSquared(me->getPosition(), player->getPosition());
	else
		return VEC3::Distance(me->getPosition(), player->getPosition());
}

void PawnUtils::movePhysics(CHandle h_collider, VEC3 move_dir, float dt, float gravity)
{
	assert(h_collider.isValid());
	TCompCollider* comp_collider = h_collider;

	static const physx::PxU32 max_shapes = 8;
	physx::PxShape* shapes[max_shapes];

	physx::PxU32 nshapes = comp_collider->getControllerNbShapes();
	assert(nshapes <= max_shapes);

	// Even when the buffer is small, it writes all the shape pointers
	if (comp_collider->is_capsule_controller)
		comp_collider->cap_controller->getActor()->getShapes(shapes, sizeof(shapes), 0);
	else
		comp_collider->box_controller->getActor()->getShapes(shapes, sizeof(shapes), 0);


	// Make a copy of the pxFilterData because the result of getQueryFilterData is returned by copy
	physx::PxFilterData filterData = shapes[0]->getQueryFilterData();
	physx::PxControllerFilters controller_filters = physx::PxControllerFilters(&filterData, &EnginePhysics.customQueryFilterCallback);
	controller_filters.mCCTFilterCallback = &CEngine::get().getPhysics().collisionFilter;

	move_dir.y += gravity * dt;

	physx::PxShape* shape = shapes[0];

	if (comp_collider->is_capsule_controller) {
		comp_collider->cap_controller->move(physx::PxVec3(move_dir.x, move_dir.y, move_dir.z), 0.0f, dt, controller_filters);
	}
	else {
		comp_collider->box_controller->move(physx::PxVec3(move_dir.x, move_dir.y, move_dir.z), 0.0f, dt, controller_filters);
		shape->setLocalPose(physx::PxTransform(physx::PxQuat(vectorToYaw(move_dir), physx::PxVec3(1, 0, 0))));
	}
}

bool PawnUtils::hasHeardPlayer(CHandle h_me, float max_hearing_dist, float hear_speed_th)
{
	CEntity* e_player = getEntityByName("player");
	if (!e_player) return false;

	TCompPlayerController* c_player_cont = e_player->get<TCompPlayerController>();

	// If the player is nearby and the speed exceeds the hearing speed threshold, return true
	return distToPlayer(h_me) <= max_hearing_dist && c_player_cont->getSpeed() >= hear_speed_th;
}

void PawnUtils::setPosition(CHandle h_me, VEC3 position)
{
	// Do a raycast to the floor to detect the floor position
	{
		std::vector<physx::PxRaycastHit> raycastHits;
		if (EnginePhysics.raycast(position, VEC3::Down, 100.f, raycastHits)) {
			position = PXVEC3_TO_VEC3(raycastHits[0].position);
		}
	}

	CEntity* pawn = h_me;
	TCompTransform* h_player_trans = pawn->get<TCompTransform>();
	TCompCollider* h_collider_player = pawn->get<TCompCollider>();
	h_player_trans->setPosition(position);
	h_collider_player->setFootPosition(position);
}

void PawnUtils::playAction(CHandle h_me, const std::string& name, float blendIn, float blendOut,
	float weight, bool root_motion, bool root_yaw, bool lock)
{
	CEntity* pawn = h_me;
	assert(pawn);
	if (!pawn)
		return;

	TCompSkeleton* c_skel = pawn->get<TCompSkeleton>();
	assert(c_skel);
	if (!c_skel)
		return;

	c_skel->playAction(name, blendIn, blendOut, weight, root_motion, root_yaw, lock);
}

void PawnUtils::playCycle(CHandle h_me, const std::string& name, float blend, float weight)
{
	CEntity* pawn = h_me;
	assert(pawn);
	if (!pawn)
		return;

	TCompSkeleton* c_skel = pawn->get<TCompSkeleton>();
	assert(c_skel);
	if (!c_skel)
		return;

	c_skel->playCycle(name, blend, weight);
}

void PawnUtils::stopAction(CHandle h_me, const std::string& name, float blendOut)
{
	CEntity* pawn = h_me;
	assert(pawn);
	if (!pawn)
		return;

	TCompSkeleton* c_skel = pawn->get<TCompSkeleton>();
	assert(c_skel);
	if (!c_skel)
		return;

	c_skel->stopAnimation(name, ANIMTYPE::ACTION, blendOut);
}

void PawnUtils::stopCycle(CHandle h_me, const std::string& name, float blendOut)
{
	CEntity* pawn = h_me;
	assert(pawn);
	if (!pawn)
		return;

	TCompSkeleton* c_skel = pawn->get<TCompSkeleton>();
	assert(c_skel);
	if (!c_skel)
		return;

	c_skel->stopAnimation(name, ANIMTYPE::CYCLE, blendOut);
}

void PawnUtils::spawnGeons(VEC3 pos, CHandle h_owner)
{
	assert(h_owner.isValid());
	CEntity* e_owner = h_owner;
	CTransform t;
	t.setPosition(pos);
	CEntity* e = spawn("data/particles/compute_geons_particles.json", t);
	TCompBuffers* buffers = e->get<TCompBuffers>();
	CShaderCte< CtesParticleSystem >* cte = static_cast<CShaderCte< CtesParticleSystem >*>(buffers->getCteByName("CtesParticleSystem"));

	TCompGeonsDrop* dropGeons = e_owner->get<TCompGeonsDrop>();
	assert(dropGeons);

	cte->emitter_initial_pos = pos;
	cte->emitter_num_particles_per_spawn = dropGeons->getGeonsDropped() * 60;
	cte->updateFromCPU();

	// FMOD event
	static const char* EVENT_NAME = "ENV/General/Drop_Eons";
	EngineAudio.postEvent(EVENT_NAME, e_owner);
}

void PawnUtils::spawnHealParticles(VEC3 pos, const std::string& name)
{
	CTransform t;
	t.setPosition(pos);

	// small particles
	CEntity* e = spawn("data/particles/compute_heal_particles.json", t);
	TCompBuffers* buffers = e->get<TCompBuffers>();
	CShaderCte< CtesParticleSystem >* cte = static_cast<CShaderCte< CtesParticleSystem >*>(buffers->getCteByName("CtesParticleSystem"));

	cte->emitter_initial_pos = t.getPosition();
	cte->emitter_num_particles_per_spawn = 2000;
	cte->updateFromCPU();

	// "smoke" particles
	e = spawn("data/particles/compute_heal_smoke_particles.json", t);
	buffers = e->get<TCompBuffers>();
	cte = static_cast<CShaderCte< CtesParticleSystem >*>(buffers->getCteByName("CtesParticleSystem"));

	cte->emitter_initial_pos = t.getPosition();
	cte->emitter_num_particles_per_spawn = 100;
	cte->updateFromCPU();
}
