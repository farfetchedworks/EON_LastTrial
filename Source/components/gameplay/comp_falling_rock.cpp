#include "mcv_platform.h"
#include "handle/handle.h"
#include "engine.h"
#include "entity/entity.h"
#include "comp_falling_rock.h"
#include "components/common/comp_transform.h"
#include "components/common/comp_collider.h"

DECL_OBJ_MANAGER("falling_rock", TCompFallingRock)

void TCompFallingRock::update(float dt)
{
	if (_isFalling)
		return;

	_timer += dt;

	if (_timer >= _waitTime) {
		TCompCollider* collider = get<TCompCollider>();
		collider->disable(false);
		physx::PxRigidDynamic* actor = (physx::PxRigidDynamic*)collider->actor;
		actor->setLinearVelocity(physx::PxVec3(0.f, -0.1f, 0.f));
		_isFalling = true;
	}
}

void TCompFallingRock::onEntityCreated()
{
	TCompCollider* collider = get<TCompCollider>();
	assert(collider);
	collider->disable(true);
}
