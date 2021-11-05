#include "mcv_platform.h"
#include "handle/handle.h"
#include "engine.h"
#include "entity/entity.h"
#include "entity/entity_parser.h"
#include "comp_destructible_prop.h"
#include "lua/module_scripting.h"
#include "audio/module_audio.h"
#include "modules/module_events.h"
#include "components/common/comp_transform.h"
#include "components/common/comp_collider.h"
#include "components/cameras/comp_camera_shake.h"

DECL_OBJ_MANAGER("destructible", TCompDestructible)

void TCompDestructible::load(const json& j, TEntityParseContext& ctx)
{
	spawn_prefab	= j.value("spawn_prefab", std::string());
	fmod_event		= j.value("fmod_event", std::string());
	callback_event	= j.value("callback_event", std::string());
	drops_warp		= j.value("drops_warp", drops_warp);
	force			= j.value("force", force);
}

void TCompDestructible::debugInMenu()
{
	if( ImGui::Button("Destroy"))
		onDestroy(TMsgPropDestroyed());
}

void TCompDestructible::onEntityCreated()
{
	h_transform = get<TCompTransform>();
}

/*
	Messages callbacks
*/

void TCompDestructible::onDestroy(const TMsgPropDestroyed& msg)
{
	// Spawn new broken mesh
	TCompTransform* transform = h_transform;
	CTransform t;
	t.fromMatrix(transform->asMatrix());
	TEntityParseContext ctx;
	spawn(spawn_prefab, t, ctx);

	for (auto& h : ctx.entities_loaded)
	{
		assert(h.isValid());
		CEntity* e = h;
		TCompCollider* collider = e->get<TCompCollider>();
		assert(collider);

		if (collider->actor->is<physx::PxRigidDynamic>())
		{
			float r = 0.4f;

			QUAT q = QUAT::CreateFromYawPitchRoll(
				Random::range(-r, r),
				Random::range(-r, r),
				Random::range(-r, r)
			);

			VEC3 dir = VEC3::Transform(msg.direction, q);
			dir.Normalize();
			collider->addForce(dir * force, "prop");

			((physx::PxRigidDynamic*)collider->actor)->setLinearDamping(physx::PxReal(1.5f));
			((physx::PxRigidDynamic*)collider->actor)->setAngularDamping(physx::PxReal(1.f));
		}

		EngineLua.executeScript("destroyEntity('" + std::string(e->getName()) + "')", Random::range(120.f, 140.f));
	}

	// Post fmod event
	if (fmod_event.length())
		EngineAudio.postEvent(fmod_event, transform->getPosition());
	if (callback_event.length())
		EventSystem.dispatchEvent(callback_event, getEntity());

	// Recover warp energy
	if (drops_warp) {
		CEntity* player = getEntityByName("player");
		assert(player);
		TMsgHitWarpRecover msgHitWarp;
		msgHitWarp.hitByPlayer = true;
		msgHitWarp.multiplier= 0.75f;
		player->sendMsg(msgHitWarp);
	}

	// Delete this
	CHandle(this).getOwner().destroy();
	CHandleManager::destroyAllPendingObjects();
}
