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
#include "components/gameplay/comp_game_manager.h"
#include "entity/entity_parser.h"

DECL_OBJ_MANAGER("eter", TCompEter)

void TCompEter::onEntityCreated()
{
	h_transform = get<TCompTransform>();
	TCompTransform* t = h_transform;

	_targetPosition = t->getPosition() + VEC3(0, 1.2f, 0);
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
	VEC3 pos = t->getPosition();
	spawnParticles("data/particles/splatter_blood_front.json", pos, pos);

	// Destruir Eter
	getEntity()->destroy();

	// Spawnear Eter Roto
	CEntity* animation_controller = spawn("data/prefabs/Eter_Broken.json", *t);
	assert(animation_controller);

	// Camara lenta
	TCompGameManager* gm = GameManager->get<TCompGameManager>();
	gm->setTimeStatusLerped(TCompGameManager::ETimeStatus::SLOW, 0.5f, &interpolators::expoInInterpolator);

	// Iniciar cinematica rotura
	// EngineLua.executeScript("CinematicEnding_2()");
	
	// Iniciar animacion rigida eter roto
	TCompRigidAnimationController* controller = animation_controller->get<TCompRigidAnimationController>();
	if (controller)
	{
		// assert(controller);
		controller->start([]() {

			// 1. Fade a negro de golpe
			EngineUI.activateWidget("modal_black", false);
			// 2. Spawnear nuevas escenas
			Boot.setEndBoot();
			// 3. Quitar modal
			EngineLua.executeScript("deactivateWidget('modal_black')", 4.f);
		});
	}

	// DEBUG---------
	EngineUI.activateWidget("modal_black", false);
	EngineLua.executeScript("deactivateWidget('modal_black')", 4.f);
	Boot.setEndBoot();
	// ---------------
}