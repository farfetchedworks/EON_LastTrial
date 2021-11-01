#include "mcv_platform.h"
#include "engine.h"
#include "entity_picker.h"
#include "entity.h"
#include "modules/module_physics.h"

bool testSceneRay(TRay ray, TEntityPickerContext& ctx)
{
	physx::PxU32 layerMask = CModulePhysics::FilterGroup::All;
	std::vector<physx::PxRaycastHit> raycastHits;
	bool hasHitRaycast = EnginePhysics.raycast(ray.origin, ray.direction, 200.f, raycastHits, layerMask, true);

	if (hasHitRaycast)
	{
		ctx.hit_position = PXVEC3_TO_VEC3(raycastHits[0].position);

		CHandle h_collider;
		h_collider.fromVoidPtr(raycastHits[0].actor->userData);
		ctx.current_entity = h_collider.getOwner();

		CEntity* e = ctx.current_entity;
		ctx.current_name = e->getName();
	}

	return hasHitRaycast;
}