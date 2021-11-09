#include "mcv_platform.h"
#include "engine.h"
#include "task_utils.h"
#include "modules/module_physics.h"
#include "bt/bt_context.h"
#include "components/common/comp_transform.h"
#include "components/common/comp_parent.h"
#include "components/controllers/comp_ai_controller_base.h"
#include "skeleton/comp_skeleton.h"
#include "skeleton/game_core_skeleton.h"
#include "skeleton/comp_skel_lookat.h"
#include "components/render/comp_dissolve.h"
#include "entity/entity_parser.h"			
#include "components/projectiles/comp_enemy_projectile.h"
#include "components/abilities/comp_area_delay.h"
#include "components/projectiles/comp_gard_branch.h"
#include "components/common/comp_fsm.h"
#include "components/gameplay/comp_lifetime.h"
#include "fsm/states/logic/state_logic.h"
#include "../bin/data/shaders/constants_particles.h"

 // #define ALLOW_PAUSES

float TaskUtils::rotation_th = 0.1f;

void TaskUtils::moveForward(CBTContext& ctx, const float speed, const float dt)
{
	TCompTransform* my_trans = ctx.getComponent<TCompTransform>();
	TCompAIControllerBase* my_cont = ctx.getComponent<TCompAIControllerBase>();

	VEC3 temptative_dir = my_trans->getForward() * speed * dt;

	my_cont->movePhysics(temptative_dir, dt);
}

VEC3 TaskUtils::moveAnyDir(CBTContext& ctx, const VEC3 dir, const float speed, const float dt, float max_dist)
{
	VEC3 temptative_dir = dir;
	temptative_dir.Normalize();
	temptative_dir = temptative_dir * speed * dt;

	// Avoid moving further than certain distance in a single frame
	if (max_dist) {
		float vel_length = temptative_dir.Length();
		vel_length = std::min(vel_length, max_dist);
		temptative_dir.Normalize();
		temptative_dir *= vel_length;
	}

	TCompAIControllerBase* my_cont = ctx.getComponent<TCompAIControllerBase>();
	my_cont->movePhysics(temptative_dir, dt);

	return temptative_dir;
}

void TaskUtils::rotateToFace(TCompTransform* my_trans, const VEC3 target, const float speed, const float dt)
{
	float delta_yaw = my_trans->getYawRotationToAimTo(target);
	CEntity* owner = CHandle(my_trans).getOwner();
	TCompAIControllerBase* controller = owner->get<TCompAIControllerBase>();
	controller->rotate(delta_yaw);
}

void TaskUtils::changeLookAtAngle(CEntity* e_observer, const float angle)
{
	TCompSkelLookAt* h_skellookat = e_observer->get<TCompSkelLookAt>();

	QUAT delta_rotation = QUAT::CreateFromAxisAngle(VEC3(0, 1, 0), angle);

	TCompTransform* c_trans = e_observer->get<TCompTransform>();
	
	VEC3 new_direction = DirectX::XMVector3Rotate(/*getBoneForward(e_observer, h_skellookat->head_bone_name)*/c_trans->getForward(), delta_rotation);
	VEC3 new_target = getBoneWorldPosition(e_observer, h_skellookat->head_bone_name) + new_direction;

	h_skellookat->setTarget(new_target);
}

float TaskUtils::rotate(TCompTransform* my_trans, const float speed, const float dt)
{
	float amount_rotated = deg2rad(10.f) * speed * dt;
	QUAT delta_rotation = QUAT::CreateFromAxisAngle(VEC3(0, 1, 0), amount_rotated);
	QUAT new_rotation = my_trans->getRotation() * delta_rotation;

	my_trans->setRotation(new_rotation);

	return amount_rotated;
}

void TaskUtils::hit(CEntity* striker, CEntity* target, int damage)
{
	TCompTransform* h_transform = striker->get<TCompTransform>();

	// Create msg to TCompHealth to reduce health
	{
		TMsgHit msgHit;
		msgHit.damage = damage;
		msgHit.h_striker = striker;
		msgHit.hitByPlayer = false;
		target->sendMsg(msgHit);

		TMsgAddForce msgForce;
		msgForce.force = h_transform->getForward() * 2.0f;
		msgForce.h_applier = striker;
		target->sendMsg(msgForce);
	}
}

bool TaskUtils::canSeePlayer(TCompTransform* my_trans, TCompTransform* player_trans)
{
	// If Eon is inside the cone, generate a raycast from the Enemy towards Eon's position
	VEC3 raycast_origin = my_trans->getPosition();
	VEC3 dir = player_trans->getPosition() - raycast_origin;
	dir.Normalize();
	VHandles colliders;
	raycast_origin.y += 1;
	float distance = 200.f;

	physx::PxU32 mask = CModulePhysics::FilterGroup::All ^ CModulePhysics::FilterGroup::Enemy ^ CModulePhysics::FilterGroup::InvisibleWall;

	bool is_ok = EnginePhysics.raycast(raycast_origin, dir, distance, colliders, mask, true);
	if (is_ok) {
		TCompCollider* c_collider = colliders[0];
		CEntity* h_firsthit = c_collider->getEntity();
		if (h_firsthit == getPlayer()) {
			return true;
		}
	}

	return false;
}

bool TaskUtils::hasObstaclesToEon(TCompTransform* my_trans, TCompTransform* player_trans)
{
	// If Eon is inside the cone, generate a raycast from the Enemy towards Eon's position
	VEC3 raycast_origin = my_trans->getPosition();
	VEC3 dir = player_trans->getPosition() - raycast_origin;
	dir.y = 0;					// To get the "forward" from the enemy to Eon
	dir.Normalize();
	VHandles colliders;
	raycast_origin.y += 1;
	float distance = 200.f;

	physx::PxU32 mask = CModulePhysics::FilterGroup::All ^ CModulePhysics::FilterGroup::Enemy;

	bool is_ok = EnginePhysics.raycast(raycast_origin, dir, distance, colliders, mask, true);
	if (is_ok) {
		TCompCollider* c_collider = colliders[0];
		CEntity* h_firsthit = c_collider->getEntity();
		if (h_firsthit == getPlayer()) {
			return false;
		}
	}

	return true;
}

void TaskUtils::spawnProjectile(const VEC3 shoot_orig, const VEC3 shoot_target, const int dmg, const bool from_player, bool is_homing)
{
	CTransform projectile_transform;
	projectile_transform.setPosition(shoot_orig);
	//projectile_transform.lookAt(shoot_orig, shoot_target + VEC3(0, 1, 0), VEC3(0, 1, 0));
	CEntity* e_projectile;
	if (!is_homing) {
		e_projectile = spawn("data/prefabs/enemy_projectile.json", projectile_transform);

	}
	else {
		e_projectile = spawn("data/prefabs/enemy_projectile_homing.json", projectile_transform);
	}
	// Set the damage to ranged_attack_dmg
	TCompEnemyProjectile* c_enemyproj = e_projectile->get<TCompEnemyProjectile>();

	VEC3 front = normVEC3((shoot_target + VEC3::Up) - shoot_orig);
	c_enemyproj->setParameters(dmg, from_player, front, is_homing);
}

void TaskUtils::spawnCygnusProjectiles(const VEC3 shoot_orig, const VEC3 shoot_target, const int dmg, const int num_projectiles)
{
	CTransform projectile_transform;
	projectile_transform.setPosition(shoot_orig);
	CEntity* e_projectile;

	// Calculate the angle of each rotation with the amount of projectiles
	float angle = 2.f * (float)M_PI / (float)num_projectiles;
	VEC3 shoot_dest = shoot_target - shoot_orig;
	shoot_dest.y = 0;
	float cos_angle = cos(angle);
	float sin_angle = sin(angle);

	for (int i = 0; i < num_projectiles; i++) {
		
		// Spawn a projectile
		e_projectile = spawn("data/prefabs/cygnus_projectile.json", projectile_transform);
		TCompEnemyProjectile* c_enemyproj = e_projectile->get<TCompEnemyProjectile>();
		
		// Define the parameters of the projectile
		VEC3 front = normVEC3(shoot_dest);
		c_enemyproj->setParameters(dmg, false, front, false);		

		// Perform a rotation on the direction vector
		VEC3 prev_shoot_dest = shoot_dest;
		shoot_dest.x = prev_shoot_dest.x * cos_angle - prev_shoot_dest.z * sin_angle;
		shoot_dest.z = prev_shoot_dest.x * sin_angle + prev_shoot_dest.z * cos_angle;
	}
}

void TaskUtils::castAreaAttack(CEntity* e_attacker, VEC3 position, const float radius, const int dmg)
{
	// Do a raycast to the floor to detect the exact position
	// so we fuck the 100% exact timings and the particles will 
	// be spawned in the floor
	{
		std::vector<physx::PxRaycastHit> raycastHits;
		if (EnginePhysics.raycast(position + VEC3::Up, VEC3::Down, 15.f, raycastHits, CModulePhysics::FilterGroup::Scenario, true)) {
			position = PXVEC3_TO_VEC3(raycastHits[0].position);
		}
	}

	spawnParticles("data/particles/gard_floor_particles.json", position, radius, 2);

	// Decal hole
	{
		CTransform t;
		t.setPosition(position);
		spawn("data/prefabs/hole_area_attack_gard.json", t);
	}

	physx::PxU32 mask = CModulePhysics::FilterGroup::Player;
	VHandles colliders;
	if (!EnginePhysics.overlapSphere(position, radius, colliders, mask))
		return;

	TCompCollider* c_collider = colliders[0];
	CEntity* e_player = c_collider->getEntity();

	TMsgHit msgHit;
	msgHit.damage = dmg;
	msgHit.h_striker = e_attacker;
	msgHit.position = position;
	msgHit.hitType = EHIT_TYPE::SMASH;

	VEC3 force_dir = e_player->getPosition() - e_attacker->getPosition();
	force_dir.Normalize();

	msgHit.forward = force_dir;
	e_player->sendMsg(msgHit);
}

VEC3 TaskUtils::getRandomPosAroundPoint(const VEC3& position, float range)
{
	return VEC3(Random::range(position.x - range, position.x + range),
				position.y,
				Random::range(position.z - range, position.z + range));
}

void TaskUtils::spawnBranch(VEC3 position, const int dmg, const float speed)
{
	// Do a raycast to the floor to detect the exact position
	{
		std::vector<physx::PxRaycastHit> raycastHits;
		if (EnginePhysics.raycast(position + VEC3::Up, VEC3::Down, 15.f, raycastHits, CModulePhysics::FilterGroup::Scenario, true)) {
			position = PXVEC3_TO_VEC3(raycastHits[0].position);
		}
	}

	CTransform branch_trans;
	branch_trans.setPosition(position);
	CEntity* e_branch = spawn("data/prefabs/gard_branch_animated.json", branch_trans);

	spawnParticles("data/particles/gard_floor_particles.json", position, 0.2f, 1);

	// Set the parameters to the branch
	TCompGardBranch* c_branch = e_branch->get<TCompGardBranch>();
	c_branch->setParameters(dmg);
}

void TaskUtils::spawnCygnusForm1Clone(VEC3 position, float lifespan)
{
	CTransform clone_trans;
	clone_trans.setPosition(position);
	CEntity* e_cygnus = spawn("data/prefabs/cygnus_form_1_clone.json", clone_trans);

	if (lifespan <= 0.f)
		lifespan = 10.f;

	TCompLifetime* c_lifetime = e_cygnus->get<TCompLifetime>();
	c_lifetime->setTTL(lifespan);
}

void TaskUtils::spawnParticles(const std::string& name, VEC3 position, float radius, int iterations, int num_particles)
{
	for (int i = 0; i < iterations; ++i) {

		CTransform t;
		t.fromMatrix(MAT44::CreateRotationY(Random::range((float)-M_PI, (float)M_PI)) *
			MAT44::CreateTranslation(position));

		TEntityParseContext ctx;
		spawn(name, t, ctx);

		for (auto h : ctx.entities_loaded) {
			CEntity* e = h;
			TCompBuffers* buffers = e->get<TCompBuffers>();
			CShaderCte< CtesParticleSystem >* cte = static_cast<CShaderCte<CtesParticleSystem>*>(buffers->getCteByName("CtesParticleSystem"));
			cte->emitter_radius = radius;

			if(num_particles != -1)
				cte->emitter_num_particles_per_spawn = num_particles;

			cte->updateFromCPU();

			TCompTransform* hTrans = e->get<TCompTransform>();
			hTrans->fromMatrix(MAT44::CreateRotationY(Random::range((float)-M_PI, (float)M_PI)) *
				MAT44::CreateTranslation(hTrans->getPosition()));
		}
	}
}

void TaskUtils::castAreaDelay(CEntity* e_caster, float duration, CHandle h_locked_t)
{
	TCompAreaDelay* h_area_delay = e_caster->get<TCompAreaDelay>();
	h_area_delay->startAreaDelay(h_locked_t, duration, true);
}

void TaskUtils::castWaveDelay(CEntity* e_caster, float initial_radius, float max_radius, float duration)
{
	TCompAreaDelay* h_area_delay = e_caster->get<TCompAreaDelay>();
	h_area_delay->startWaveDelay(initial_radius, max_radius, duration);
}

// animations ------------------------------------------------------------

void TaskUtils::setSkelLookAtEnabled(CEntity* e, bool enabled)
{
	assert(e);
	if (!e)
		return;
	
	TCompSkelLookAt* look_at = e->get<TCompSkelLookAt>();
	assert(look_at);
	if (!look_at)
		return;

	look_at->setEnabled(enabled);
}

void TaskUtils::playAction(CEntity* owner, const std::string& name, float blendIn, float blendOut,
	float weight, bool root_motion, bool root_yaw, bool lock)
{
	assert(owner);
	if (!owner)
		return;

	TCompSkeleton* c_skel = owner->get<TCompSkeleton>();
	assert(c_skel);
	if (!c_skel)
		return;

	c_skel->playAction(name, blendIn, blendOut, weight, root_motion, root_yaw, lock);
}

void TaskUtils::playCycle(CEntity* owner, const std::string& name, float blend, float weight)
{
	assert(owner);
	if (!owner)
		return;

	TCompSkeleton* c_skel = owner->get<TCompSkeleton>();
	assert(c_skel);
	if (!c_skel)
		return;

	c_skel->playCycle(name, blend, weight);
}

fsm::CContext& TaskUtils::getFSMCtx(CBTContext& ctx)
{
	TCompFSM* c_fsm = ctx.getComponent<TCompFSM>();
	assert(c_fsm);
	return c_fsm->getCtx();
}

void TaskUtils::pauseAction(CBTContext& ctx, const std::string& name, std::function<void(CBTContext& ctx)> cb)
{
#ifdef ALLOW_PAUSES
	fsm::CContext& fsmCtx = getFSMCtx(ctx);
	fsmCtx.setEnabled(false);
	auto current_state = (fsm::CStateBaseLogic*)fsmCtx.getCurrentState();
	current_state->getAnimation().pause(fsmCtx);
	ctx.setNodeVariable(name, "pause_animation", true);

	if (cb) {
		ctx.setNodeVariable(name, "pause_callback", cb);
	}
#else
	if (cb) {
		cb(ctx);
	}
#endif
}

void TaskUtils::resumeAction(CBTContext& ctx, const std::string& name)
{
#ifdef ALLOW_PAUSES
	fsm::CContext& fsmCtx = getFSMCtx(ctx);
	fsmCtx.setEnabled(true);
	auto current_state = (fsm::CStateBaseLogic*)fsmCtx.getCurrentState();
	current_state->getAnimation().resume(fsmCtx);

	if (ctx.isNodeVariableValid(name, "pause_callback")) {
		auto cb = ctx.getNodeVariable<std::function<void(CBTContext& ctx)>>(name, "pause_callback");
		if (cb) cb(ctx);
		ctx.setNodeVariable(name, "pause_callback", nullptr);
	}
#endif
}

void TaskUtils::resumeActionAt(CBTContext& ctx, const std::string& name, float time, float dt)
{
#ifdef ALLOW_PAUSES
	bool animation_paused = false;
	if (ctx.isNodeVariableValid(name, "pause_animation"))
		animation_paused = ctx.getNodeVariable<bool>(name, "pause_animation");

	if (animation_paused) {

		float curr_time = 0.f;
		if (ctx.isNodeVariableValid(name, "dt_acum"))
			curr_time = ctx.getNodeVariable<float>(name, "dt_acum");
		
		curr_time += dt;

		ctx.setNodeVariable(name, "dt_acum", curr_time);

		if (curr_time >= time) {
			ctx.setNodeVariable(name, "pause_animation", false);
			ctx.setNodeVariable(name, "dt_acum", 0.f);
			TaskUtils::resumeAction(ctx, name);
		}
	}
#endif
}

void TaskUtils::stopAction(CEntity* owner, const std::string& name, float blendOut)
{
	assert(owner);
	if (!owner)
		return;

	TCompSkeleton* c_skel = owner->get<TCompSkeleton>();
	assert(c_skel);
	if (!c_skel)
		return;

	auto core = (CGameCoreSkeleton*)c_skel->model->getCoreModel();
	int anim_id = core->getCoreAnimationId(name);
	auto mixer = c_skel->model->getMixer();
	mixer->removeAction(anim_id, blendOut);
}

void TaskUtils::stopCycle(CEntity* owner, const std::string& name, float blendOut)
{
	assert(owner);
	if (!owner)
		return;

	TCompSkeleton* c_skel = owner->get<TCompSkeleton>();
	assert(c_skel);
	if (!c_skel)
		return;

	auto core = (CGameCoreSkeleton*)c_skel->model->getCoreModel();
	int anim_id = core->getCoreAnimationId(name);
	auto mixer = c_skel->model->getMixer();
	mixer->clearCycle(anim_id, blendOut);
}

// ---------------------------------------------------------------------

void TaskUtils::dissolveAt(CBTContext& ctx, float time, float wait_time, bool propagate_childs)
{
	CEntity* e_owner = ctx.getOwnerEntity();
	TCompDissolveEffect* c_dissolve = e_owner->get<TCompDissolveEffect>();
	if (c_dissolve) {
		c_dissolve->enable(time, wait_time, propagate_childs);
	}
}

// ---------------------------------------------------------------------

VEC3 TaskUtils::getBoneWorldPosition(CHandle entity, const std::string& bone_name)
{
	assert(entity.isValid());
	CEntity* owner = entity;
	TCompSkeleton* c_skel = owner->get<TCompSkeleton>();
	assert(c_skel);
	int bone_id = c_skel->getBoneIdByName(bone_name);
	assert(bone_id != -1);
	CTransform t = c_skel->getWorldTransformOfBone(bone_id);
	return t.getPosition();
}

VEC3 TaskUtils::getBoneForward(CHandle entity, const std::string& bone_name)
{
	assert(entity.isValid());
	CEntity* owner = entity;
	TCompSkeleton* c_skel = owner->get<TCompSkeleton>();
	assert(c_skel);
	CTransform t = c_skel->getWorldTransformOfBone(bone_name, true); // correct_mcv axis
	return t.getForward();
}