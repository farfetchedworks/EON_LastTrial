#include "mcv_platform.h"
#include "handle/handle.h"
#include "engine.h"
#include "entity/entity.h"
#include "comp_eter.h"
#include "lua/module_scripting.h"
#include "ui/ui_module.h"
#include "modules/module_boot.h"
#include "components/common/comp_transform.h"
#include "components/common/comp_collider.h"
#include "components/controllers/comp_rigid_animation_controller.h"
#include "components/postfx/comp_motion_blur.h"
#include "components/gameplay/comp_game_manager.h"
#include "entity/entity_parser.h"

DECL_OBJ_MANAGER("eter", TCompEter)

void TCompEter::onEntityCreated()
{
	h_transform = get<TCompTransform>();
	TCompTransform* t = h_transform;

	_targetPosition = t->getPosition() + VEC3(0, 1.5f, 0);
}

void TCompEter::update(float dt)
{
	TCompTransform* t = h_transform;

	if (t->getPosition() != _targetPosition)
	{
		t->setPosition( damp<VEC3>(t->getPosition(), _targetPosition, 1.25f, dt) );
	}
}

void TCompEter::onHit()
{
	// Manage ENDING TWO
	TCompTransform* t = h_transform;
	VEC3 spawnPos = t->getPosition();
	spawnParticles("data/particles/splatter_blood_front.json", spawnPos, spawnPos);

	// Destruir Eter
	getEntity()->destroy();

	// Spawnear Eter Roto
	CEntity* animation_controller = spawn("data/prefabs/Eter_Broken.json", *t);
	assert(animation_controller);

	// Iniciar cinematica rotura
	EngineLua.executeScript("CinematicEnding_2()");
	
	// Iniciar animacion rigida eter roto
	TCompRigidAnimationController* controller = animation_controller->get<TCompRigidAnimationController>();
	assert(controller);
	controller->setAnimation("data/animations/Eter_Broken_Anim.anim");

	// Camara lenta
	controller->addEventTimestamp("slow_time", 88, [](){
		TCompGameManager* gm = GameManager->get<TCompGameManager>();
		gm->setTimeStatusLerped(TCompGameManager::ETimeStatus::SLOWEST, 0.5f, &interpolators::expoOutInterpolator);
	});

	// Motion Blur
	controller->addEventTimestamp("motion_blur", 70, []() {
		CEntity* camera = getEntityByName("camera_mixed");
		assert(camera);
		TCompMotionBlur* c_motion_blur = camera->get<TCompMotionBlur>();
		if (c_motion_blur) {
			c_motion_blur->enable();
		}
	});

	// Blood
	controller->addEventTimestamp("blood", 86, [spawnPos]() {
		spawnParticles("data/particles/splatter_blood_front.json", spawnPos, spawnPos);
	});

	// End and happy room
	controller->addEventTimestamp("reset", 110, []() {
		EngineUI.activateWidget("modal_black", false);
		// 2. Spawnear nuevas escenas
		Boot.setEndBoot();
		// 3. Quitar modal
		EngineLua.executeScript("activateWidget('modal_black')", 4.f);
	});

	// EngineLua.executeScript("activateWidget('modal_black')", 4.f);
	controller->start();
}