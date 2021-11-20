#include "mcv_platform.h"
#include "handle/handle.h"
#include "engine.h"
#include "entity/entity.h"
#include "comp_aether.h"
#include "components/common/comp_transform.h"
#include "components/common/comp_collider.h"

DECL_OBJ_MANAGER("aether", TCompAether)

void TCompAether::onEntityCreated()
{
	h_transform = get<TCompTransform>();
	TCompTransform* t = h_transform;

	_finalPose = t->getPosition() + VEC3(0, 0.8f, 0);
}

void TCompAether::update(float dt)
{
	TCompTransform* t = h_transform;

	if (t->getPosition() != _finalPose)
	{
		t->setPosition( damp<VEC3>(t->getPosition(), _finalPose, 1.f, dt) );
	}
}

void TCompAether::onHit()
{
	TCompTransform* t = h_transform;
	if (t->getPosition() != _finalPose)
		return;

	VEC3 pos = t->getPosition() + VEC3(0, 0.5f, 0);
	spawnParticles("data/particles/splatter_blood_front.json", pos, pos);

	CHandle(this).getOwner().destroy();
}