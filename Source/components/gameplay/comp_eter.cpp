#include "mcv_platform.h"
#include "handle/handle.h"
#include "engine.h"
#include "entity/entity.h"
#include "comp_eter.h"
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

	// Iniciar animacion rigida eter roto
	TCompRigidAnimationController* controller = animation_controller->get<TCompRigidAnimationController>();
	if (controller)
	{
		// assert(controller);
		controller->start();
	}

	// Camara lenta
	TCompGameManager* gm = GameManager->get<TCompGameManager>();
	gm->setTimeStatusLerped(TCompGameManager::ETimeStatus::SLOW, 0.5f, &interpolators::expoInInterpolator);

	// Iniciar cinematica rotura

	// Cuando la cinematica acabe:
	// 1. Fade a negro de golpe
	// 2. Borrar TODO
	// 3. Esperar unos segundos (deberia haber musica de tic tac mientras)
	// 4. Spawnear la happy room 
	// 5. Dejar mover al player hasta que se vaya x la puerta (interaccion)?
}