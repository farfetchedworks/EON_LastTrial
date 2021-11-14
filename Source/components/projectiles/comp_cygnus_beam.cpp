#include "mcv_platform.h"
#include "engine.h"
#include "components/projectiles/comp_cygnus_beam.h"
#include "components/common/comp_transform.h"
#include "modules/module_physics.h"
#include "lua/module_scripting.h"
#include "entity/entity_parser.h"
#include "../bin/data/shaders/constants_particles.h"
#include "modules/module_physics.h"

DECL_OBJ_MANAGER("cygnus_beam", TCompCygnusBeam)

void TCompCygnusBeam::load(const json& j, TEntityParseContext& ctx)
{
	damage = j.value("damage", damage);
	offset = loadVEC3(j, "offset");
}

void TCompCygnusBeam::setParameters(int new_damage)
{
	damage = new_damage;
}

void TCompCygnusBeam::debugInMenu()
{
	ImGui::DragInt("Damage", &damage, 5, 1, 100);
}

void TCompCygnusBeam::onEntityCreated()
{
	h_collider = get<TCompCollider>();
	h_transform = get<TCompTransform>();
}

void TCompCygnusBeam::update(float dt)
{
	// Apply the offset directly to the beam
	TCompCollider* c_collider = h_collider;
	TCompTransform* c_transform = h_transform;
	assert(c_collider && c_transform);
	assert(c_collider->actor);

	CTransform t = static_cast<CTransform>(*c_transform);

	CTransform t2;
	t2.fromMatrix(MAT44::CreateTranslation(offset));
	t = t.combinedWith(t2);

	physx::PxTransform pose = toPxTransform(t);

	c_collider->actor->setGlobalPose(pose);

}

// When the projectile hits something, sends a message to the player and gets destroyed
void TCompCygnusBeam::onTriggerEnter(const TMsgEntityTriggerEnter& msg)
{
	CEntity* e_object_hit = msg.h_entity;
	if (!e_object_hit)
		return;

	TCompTransform* t = get<TCompTransform>();

	TMsgHit msgHit;
	msgHit.damage = damage;
	msgHit.h_striker = CHandle(this).getOwner();
	msgHit.position = t->getPosition();
	msgHit.forward = t->getForward();
	msgHit.hitByProjectile = true;
	e_object_hit->sendMsg(msgHit);
}

void TCompCygnusBeam::destroy()
{
	//CHandle(this).getOwner().destroy();
	//
	//CEntity* e = trail;
	//// Alex: Porque podria no haber?
	//if (!e)
	//	return;

	//// Stop the emitter
	//{
	//	TCompBuffers* buffers = e->get<TCompBuffers>();
	//	CShaderCte< CtesParticleSystem >* cte = static_cast<CShaderCte<CtesParticleSystem>*>(buffers->getCteByName("CtesParticleSystem"));
	//	cte->emitter_stopped = 1.f;
	//	cte->updateFromCPU();
	//}

	//// Destroy trail in x secs
	//{
	//	std::string name = e->getName();
	//	std::string argument = "destroyEntity('" + name + "')";
	//	EngineLua.executeScript(argument, fabsf(lifetime - current_time));
	//}
}