#include "mcv_platform.h"
#include "engine.h"
#include "components/projectiles/comp_enemy_projectile.h"
#include "components/common/comp_transform.h"
#include "modules/module_physics.h"
#include "lua/module_scripting.h"
#include "entity/entity_parser.h"
#include "../bin/data/shaders/constants_particles.h"

DECL_OBJ_MANAGER("enemy_projectile", TCompEnemyProjectile)

void TCompEnemyProjectile::load(const json& j, TEntityParseContext& ctx)
{
	damage = j.value("damage", damage);
	lifetime = j.value("lifetime", lifetime);
	speed = j.value("speed", speed);
	is_homing = j.value("is_homing", is_homing);
	duration = j.value("duration", duration);
	speed_reduction = j.value("speed_reduction", speed_reduction);
}

void TCompEnemyProjectile::setParameters(int new_damage, bool new_from_player, VEC3 new_target, bool new_is_homing)
{
	damage = new_damage;
	from_player = new_from_player;
	is_homing = new_is_homing;

	TCompTransform* c_t = projectile_transform;

	if (!is_homing) {
		trail = spawn("data/particles/enemy_projectile_trail.json", *c_t);
		front = new_target;
	}
	else {
		trail = spawn("data/particles/enemy_projectile_homing_trail.json", *c_t);
		target = getEntityByName("player");
	}

	assert(trail.isValid());

	initial_scale = c_t->getScale();
	c_t->setScale(VEC3::Zero);

	CEntity* e = trail;
	TCompBuffers* buffers = e->get<TCompBuffers>();
	CShaderCte< CtesParticleSystem >* cte = static_cast<CShaderCte<CtesParticleSystem>*>(buffers->getCteByName("CtesParticleSystem"));
	cte->emitter_initial_pos = e->getPosition();
	cte->updateFromCPU();
}

void TCompEnemyProjectile::debugInMenu()
{
	ImGui::DragInt("Damage", &damage, 5, 1, 100);
	ImGui::DragFloat("Lifetime", &lifetime, 5.f, 0.f, 20.f);
	ImGui::DragFloat("Speed", &speed, 5.f, 1.f, 100.f);
}

void TCompEnemyProjectile::onEntityCreated()
{
	projectile_transform = get<TCompTransform>();
	
	/*
		 The entity containing the projectile must be a trigger AND a rigid body. If it is static, it cannot be moved according to its parent
		 Gravity must be disabled on the rigid body, or the projectile will fall
	*/
	projectile_collider = get<TCompCollider>();
	TCompCollider* c_collider = projectile_collider;
	physx::PxRigidDynamic* actor_collider = static_cast<physx::PxRigidDynamic*>(c_collider->actor);
	actor_collider->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, true);
}

void TCompEnemyProjectile::update(float dt)
{
	// Timer to check if the projectile has to be destroyed
	current_time += dt;							

	// Apply the speed multiplier to the dt if the object is affected by area delay
	dt *= speed_multiplier;			

	move(dt);

	// Scale projectile
	{
		TCompTransform* c_transform = projectile_transform;
		c_transform->setScale(initial_scale * clampf(powf(current_time, 0.15f), 0.2f, 1.f));
	}

	// Destroy projectile after a certain time if it didn't hit anything
	if (current_time > lifetime)
		destroy();
}


void TCompEnemyProjectile::move(float dt)
{
	TCompTransform* c_transform = projectile_transform;
	CTransform destination_trans;

	// Move the projectile with its forward
	VEC3 next_pos = front * speed * dt;

	if (is_homing) {
		CEntity* e_target = target;
		if (!e_target) return;

		TCompTransform* c_target_trans = e_target->get<TCompTransform>();
		next_pos = normVEC3(c_target_trans->getPosition() + VEC3::Up - c_transform->getPosition());
		next_pos *= speed * dt;
	}

	// Always set the next position to advance and the current rotation
	destination_trans.setPosition(c_transform->getPosition() + next_pos);
	
	// Call setKinematicTarget to move kinematic objects (they do not move like controllers)
	TCompCollider* c_collider = projectile_collider;
	physx::PxTransform newTransform = toPxTransform(destination_trans);
	((physx::PxRigidDynamic*)c_collider->actor)->setGlobalPose(newTransform);

	// Update trail
	if (!trail.isValid())
		return;
	CEntity* e = trail;
	TCompBuffers* buffers = e->get<TCompBuffers>();
	CShaderCte< CtesParticleSystem >* cte = static_cast<CShaderCte<CtesParticleSystem>*>(buffers->getCteByName("CtesParticleSystem"));
	cte->emitter_initial_pos = destination_trans.getPosition();
	cte->updateFromCPU();
}

// When the projectile hits something, sends a message to the player and gets destroyed
void TCompEnemyProjectile::onHitObject(const TMsgEntityOnContact& msg)
{
	CEntity* e_object_hit = msg.h_entity;
	if (!e_object_hit)
		return;

	TCompTransform* t = get<TCompTransform>();

	// If the projectile is homing, apply a wave delay. Otherwise, damage the player
	if (is_homing) {
		TMsgApplyAreaDelay msgApplyAreaDelay;
		msgApplyAreaDelay.speedMultiplier = 1.f / speed_reduction;
		msgApplyAreaDelay.isWave = true;
		msgApplyAreaDelay.waveDuration = duration;
		e_object_hit->sendMsg(msgApplyAreaDelay);
	}
	else {
		TMsgHit msgHit;
		msgHit.damage = damage;
		msgHit.h_striker = CHandle(this).getOwner();
		msgHit.position = t->getPosition();
		msgHit.forward = t->getForward();
		msgHit.hitByProjectile = true;
		msgHit.hitByPlayer = from_player;
		e_object_hit->sendMsg(msgHit);
	}

	// Destroy projectile when it hits anything
	destroy();

}

void TCompEnemyProjectile::destroy()
{
	CHandle(this).getOwner().destroy();
	
	CEntity* e = trail;
	// Alex: Porque podria no haber?
	if (!e)
		return;

	// Stop the emitter
	{
		TCompBuffers* buffers = e->get<TCompBuffers>();
		CShaderCte< CtesParticleSystem >* cte = static_cast<CShaderCte<CtesParticleSystem>*>(buffers->getCteByName("CtesParticleSystem"));
		cte->emitter_stopped = 1.f;
		cte->updateFromCPU();
	}

	// Destroy trail in x secs
	{
		std::string name = e->getName();
		std::string argument = "destroyEntity('" + name + "')";
		EngineLua.executeScript(argument, fabsf(lifetime - current_time));
	}
}

void TCompEnemyProjectile::onApplyAreaDelay(const TMsgApplyAreaDelay& msg)
{
	speed_multiplier = msg.speedMultiplier;
}

void TCompEnemyProjectile::onRemoveAreaDelay(const TMsgRemoveAreaDelay& msg)
{
	speed_multiplier = 1.f;
}