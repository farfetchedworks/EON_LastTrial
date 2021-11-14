#include "mcv_platform.h"
#include "engine.h"
#include "components/projectiles/comp_cygnus_beam.h"
#include "components/common/comp_transform.h"
#include "modules/module_physics.h"
#include "lua/module_scripting.h"
#include "entity/entity_parser.h"
#include "../bin/data/shaders/constants_particles.h"

DECL_OBJ_MANAGER("cygnus_beam", TCompCygnusBeam)

void TCompCygnusBeam::load(const json& j, TEntityParseContext& ctx)
{
	damage = j.value("damage", damage);
}

void TCompCygnusBeam::setParameters(int new_damage)
{
	damage = new_damage;
	//assert(trail.isValid());

	//CEntity* e = trail;
	//TCompBuffers* buffers = e->get<TCompBuffers>();
	//CShaderCte< CtesParticleSystem >* cte = static_cast<CShaderCte<CtesParticleSystem>*>(buffers->getCteByName("CtesParticleSystem"));
	//cte->emitter_initial_pos = e->getPosition();
	//cte->updateFromCPU();
}

void TCompCygnusBeam::debugInMenu()
{
	ImGui::DragInt("Damage", &damage, 5, 1, 100);
}

void TCompCygnusBeam::onEntityCreated()
{}

void TCompCygnusBeam::update(float dt)
{}

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