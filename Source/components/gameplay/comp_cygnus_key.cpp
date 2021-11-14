#include "mcv_platform.h"
#include "engine.h"
#include "entity/entity.h"
#include "comp_cygnus_key.h"
#include "entity/entity_parser.h"
#include "audio/module_audio.h"
#include "input/input_module.h"
#include "lua/module_scripting.h"
#include "skeleton/comp_skeleton.h"
#include "components/common/comp_parent.h"
#include "components/common/comp_render.h"
#include "components/common/comp_collider.h"
#include "components/controllers/comp_rigid_animation_controller.h"
#include "../bin/data/shaders/constants_particles.h"

DECL_OBJ_MANAGER("cygnus_key", TCompCygnusKey)

void TCompCygnusKey::load(const json& j, TEntityParseContext& ctx)
{
	_order = j.value("order", _order);
}

void TCompCygnusKey::onAllEntitiesCreated(const TMsgAllEntitiesCreated& msg)
{
	CEntity* e = getEntity();
	int nb = sscanf(e->getName(), "CygnusKey_%d", &_order);
	assert(nb > 0);

	if (_order == 0)
		setActive();
}

void TCompCygnusKey::update(float dt)
{
#ifdef _DEBUG
	if (PlayerInput['O'].getsPressed())
		onAllKeysOpened();
#endif // _DEBUG

	if (_waitTime > 0.f)
	{
		_waitTime -= dt;

		if (_waitTime < 0.f)
		{
			setActive();
		}
	}
}

bool TCompCygnusKey::resolve()
{
	if (!_active)
	{
		// Not the correct..
		return false;
	}

	// This is about to be destroyed:
	// 1. Manage which is the next Key

	CHandle next;
	int min = 1000;

	getObjectManager<TCompCygnusKey>()->forEach([&](TCompCygnusKey* key) {
		// discard me
		if (key && key != CHandle(this) && key->getOrder() < min) {
			min = key->getOrder();
			next = key;
		}
	});

	// It was the last
	if (!next.isValid())
	{
		onAllKeysOpened();
		return true;
	}

	// 2. Set new active
	TCompCygnusKey* key = next;
	CEntity* newK = next.getOwner();
	CEntity* owner = getEntity();
	float distance = VEC3::Distance(newK->getPosition(), getEntity()->getPosition());

	key->start(2.f + distance / 12.f);

	// 3. Spawn geons
	CTransform t;
	t.setPosition(owner->getPosition());
	CEntity* e = spawn("data/particles/compute_direct_geons_particles.json", t);
	TCompBuffers* buffers = e->get<TCompBuffers>();
	CShaderCte< CtesParticleSystem >* cte = static_cast<CShaderCte< CtesParticleSystem >*>(buffers->getCteByName("CtesParticleSystem"));

	cte->emitter_initial_pos = owner->getPosition();
	cte->emitter_owner_position = static_cast<CEntity*>(next.getOwner())->getPosition();
	cte->emitter_num_particles_per_spawn = 1800;
	cte->updateFromCPU();

	// FMOD event
	static const char* EVENT_NAME = "ENV/General/Drop_Eons";
	EngineAudio.postEvent(EVENT_NAME, owner);

	return true;
}

void TCompCygnusKey::start(float time)
{
	_waitTime = time;
}

void TCompCygnusKey::setActive()
{
	// Enable fire
	TCompParent* parent = get<TCompParent>();
	CEntity* fire = parent->getChildByName("Fire_Active");
	assert(fire);
	TCompRender* render = fire->get<TCompRender>();
	render->setEnabled(true);

	// Active idle
	TCompSkeleton* c_skel = get<TCompSkeleton>();
	assert(c_skel);
	c_skel->playAnimation("CygnusKey_active", ANIMTYPE::CYCLE);

	// Do this when geons arrive?
	// How can I detect it...???
	c_skel->playAnimation("CygnusKey_charge", ANIMTYPE::ACTION);

	// Set new mask
	TCompCollider* collider = get<TCompCollider>();
	collider->setGroupAndMask("enemy", "all");

	_active = true;
}

void TCompCygnusKey::onAllKeysOpened()
{
	if (_keysOpened)
		return;

	// Play animation
	{
		CEntity* e = getEntityByName("Sculptures_Broken");
		assert(e);
		TCompRigidAnimationController* controller = e->get<TCompRigidAnimationController>();
		assert(controller);
		controller->start();
		EngineLua.executeScript("shakeOnce(2, 2, 9)");
	}

	// Destroy collider

	CEntity* e = getEntityByName("Sculptures_Broken_Collider");
	assert(e);
	
	{
		TCompCollider* collider = e->get<TCompCollider>();
		assert(collider);
		collider->setGroupAndMask("none", "none");
		e->destroy();
	}

	// Spawn complete colliders
	{
		CTransform t;
		t.setPosition(e->getPosition());
		spawn("data/prefabs/Sculptures_Complete_Colliders.json", t);
	}

	_keysOpened = true;
}

void TCompCygnusKey::debugInMenu()
{
	ImGui::DragInt("Order", &_order);
	ImGui::Checkbox("Active", &_active);

	if(ImGui::Button("Debug All opened"))
	{
		onAllKeysOpened();
	}

}