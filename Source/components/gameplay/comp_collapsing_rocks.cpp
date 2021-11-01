#include "mcv_platform.h"
#include "handle/handle.h"
#include "engine.h"
#include "entity/entity.h"
#include "entity/entity_parser.h"
#include "comp_collapsing_rocks.h"
#include "comp_falling_rock.h"
#include "components/common/comp_transform.h"
#include "components/common/comp_collider.h"
#include "components/messages.h"
#include "components/cameras/comp_camera_shake.h"
#include "lua/module_scripting.h"
#include "audio/module_audio.h"

DECL_OBJ_MANAGER("collapsing_rocks", TCompCollapsingRocks)

void TCompCollapsingRocks::load(const json& j, TEntityParseContext& ctx)
{
	spawn_prefab = "";
	spawn_prefab = j.value("spawn_prefab", spawn_prefab);
	fall_time = j.value("fall_time", fall_time);
}

void TCompCollapsingRocks::debugInMenu()
{
	if (ImGui::Button("Trigger"))
		onPlayerStepOn(TMsgEntityTriggerEnter());
}

/*
	Messages callbacks
*/

void TCompCollapsingRocks::onPlayerStepOn(const TMsgEntityTriggerEnter& msg)
{
	CEntity* e_player = getEntityByName("player");
	CEntity* e_object_hit = e_player;// msg.h_entity;

	if (e_object_hit != e_player)
		return;

	TCompTransform* playerTransform = e_player->get<TCompTransform>();
	TCompTransform* transform = get<TCompTransform>();
	TEntityParseContext ctx;
	spawn(spawn_prefab, *transform, ctx);

	float maxDistance = -1.f;

	// Get max rock distance
	for (auto& h : ctx.entities_loaded)
	{
		assert(h.isValid());
		CEntity* e_rock = h;

		TCompTransform* rockTransform = e_rock->get<TCompTransform>();
		float distToPlayer = rockTransform->distance(*playerTransform);

		maxDistance = std::max(distToPlayer, maxDistance);
	}

	// Use max distance to compute falling wait time
	for (auto& h : ctx.entities_loaded)
	{
		assert(h.isValid());
		CEntity* e_rock = h;
		TCompFallingRock* rock = e_rock->get<TCompFallingRock>();
		assert(rock);
		if (!rock)
			continue;

		TCompCollider* collider = e_rock->get<TCompCollider>();
		assert(collider);

		((physx::PxRigidDynamic*)collider->actor)->setLinearDamping(physx::PxReal(5.f));
		((physx::PxRigidDynamic*)collider->actor)->setAngularDamping(physx::PxReal(0.5f));

		TCompTransform* rockTransform = e_rock->get<TCompTransform>();
		float distToPlayer = rockTransform->distance(*playerTransform);
		rock->setWaitTime(powf(distToPlayer / maxDistance, 4.f) * 3.f);

		EngineLua.executeScript("destroyEntity('" + std::string(e_rock->getName()) + "')", Random::range(10.f, 30.f));
	}

	CEntity* outputCamera = getEntityByName("camera_mixed");
	TCompCameraShake* shaker = outputCamera->get<TCompCameraShake>();
	shaker->shakeOnce(1.5f, 0.25f, 0.1f);

	// call Lua to shake camera in "fall_time" seconds
	EngineLua.executeScript("shakeOnce(4, 0.1, 2.5)", fall_time);

	// post fmod event
	EngineAudio.postEvent("ENV/Cave/Rocks_Falling", transform->getPosition());

	// delete this
	CHandle(this).getOwner().destroy();
	CHandleManager::destroyAllPendingObjects();
}