#include "mcv_platform.h"
#include "handle/handle.h"
#include "engine.h"
#include "entity/entity.h"
#include "comp_eter.h"
#include "lua/module_scripting.h"
#include "ui/ui_module.h"
#include "modules/module_boot.h"
#include "input/input_module.h"
#include "components/common/comp_transform.h"
#include "components/common/comp_collider.h"
#include "components/controllers/comp_rigid_animation_controller.h"
#include "components/postfx/comp_motion_blur.h"
#include "components/gameplay/comp_game_manager.h"
#include "entity/entity_parser.h"

DECL_OBJ_MANAGER("eter", TCompEter)

extern CShaderCte<CtesWorld> cte_world;

void TCompEter::load(const json& j, TEntityParseContext& ctx)
{
	_isBroken = j.value("isBroken", _isBroken);
}

void TCompEter::onEntityCreated()
{
	h_transform = get<TCompTransform>();
	TCompTransform* t = h_transform;

	_initialPosition = t->getPosition() - VEC3(0, 0.5f, 0);;
	_targetPosition = t->getPosition() + VEC3(0, 1.5f, 0);
}

void TCompEter::update(float dt)
{
	if (_isBroken)
	{
		if(_exploded)
			cte_world.exposure_factor = damp(cte_world.exposure_factor, 0.1f, 8.f, Time.delta_unscaled);
		return;
	}

	TCompTransform* t = h_transform;

	if (t->getPosition() != _targetPosition)
	{
		t->setPosition( damp<VEC3>(t->getPosition(), _targetPosition, 1.25f, dt) );
	}
	
	if(!_spawned && VEC3::Distance(t->getPosition(), _targetPosition) < 0.1f)
	{
		spawnBrokenEter();
	}
}

void TCompEter::spawnBrokenEter()
{
	_spawned = true;

	TCompTransform* t = h_transform;
	VEC3 spawnPos = t->getPosition();
	spawnParticles("data/particles/splatter_blood_front.json", spawnPos, spawnPos);

	QUAT rot = t->getRotation();

	// Spawnear Eter Roto
	CTransform tb;
	tb.fromMatrix(*t);
	CEntity* animation_controller = spawn("data/prefabs/Eter_Broken.json", *t);
	assert(animation_controller);

	TCompRigidAnimationController* controller = animation_controller->get<TCompRigidAnimationController>();
	assert(controller);
	controller->setAnimation("data/animations/Eter_Broken_Idle_Anim.anim");
	controller->start();

	EngineLua.executeScript("shake(0.1, 1.0)");

	// Destruir Eter
	getEntity()->destroy();
}

void TCompEter::onHit()
{
	// *****************
	// Manage ENDING TWO
	// *****************

	_exploded = true;

	TCompTransform* t = h_transform;
	t->setPosition(_initialPosition);

	// Spawnear Eter Roto
	CEntity* animation_controller = getEntityByName("Eter_Controller");
	assert(animation_controller);
	TCompRigidAnimationController* controller = animation_controller->get<TCompRigidAnimationController>();
	assert(controller);
	controller->setLoop(false);
	controller->setAnimation("data/animations/Eter_Broken_Explosion_Anim.anim");

	// Camara lenta
	controller->addEventTimestamp("slow_time", 3, [](){
		TCompGameManager* gm = GameManager->get<TCompGameManager>();
		gm->setTimeStatusLerped(TCompGameManager::ETimeStatus::SLOWEST, 1.0f, &interpolators::expoOutInterpolator);
		// Iniciar cinematica rotura
		EngineLua.executeScript("CinematicEnding_2()");
	});

	// Motion Blur
	controller->addEventTimestamp("motion_blur", 0, []() {
		CEntity* camera = getEntityByName("camera_mixed");
		assert(camera);
		TCompMotionBlur* c_motion_blur = camera->get<TCompMotionBlur>();
		if (c_motion_blur) {
			c_motion_blur->enable();
		}
	});

	// Blood
	controller->addEventTimestamp("blood", 5, [&]() {
		TCompTransform* t = h_transform;
		VEC3 spawnPos = t->getPosition();
		spawnParticles("data/particles/compute_ether_explosion_particles.json", spawnPos, spawnPos);
	});

	// End and happy room
	controller->addEventTimestamp("reset", 12, []() {
		EngineUI.activateWidget("modal_black", false);

		TCompGameManager* gm = GameManager->get<TCompGameManager>();
		gm->setTimeStatus(TCompGameManager::ETimeStatus::NORMAL);

		EngineLua.executeScript("deactivateWidget('modal_black')", 6.0f);

		Boot.setEndBoot();

		PlayerInput.unBlockInput();
	});

	// Shake camera a little bit (will be reduced with the slow time..)
	EngineLua.executeScript("shakeOnce(15, 0.0, 10.0)");

	controller->start();
}