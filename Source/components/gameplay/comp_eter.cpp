#include "mcv_platform.h"
#include "handle/handle.h"
#include "engine.h"
#include "entity/entity.h"
#include "comp_eter.h"
#include "components/common/comp_transform.h"
#include "components/common/comp_collider.h"

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
	// Manage ENDING TWO: eter is broken and go to happy room
	TCompTransform* t = h_transform;
	VEC3 pos = t->getPosition() + VEC3(0, 0.5f, 0);
	spawnParticles("data/particles/splatter_blood_front.json", pos, pos);

	CHandle(this).getOwner().destroy();
}