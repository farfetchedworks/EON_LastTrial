#include "mcv_platform.h"
#include "handle/handle.h"
#include "engine.h"
#include "entity/entity.h"
#include "entity/entity_parser.h"
#include "comp_collapsing_props.h"
#include "comp_falling_rock.h"
#include "components/common/comp_transform.h"
#include "components/common/comp_collider.h"
#include "components/messages.h"
#include "components/cameras/comp_camera_shake.h"
#include "lua/module_scripting.h"
#include "audio/module_audio.h"

DECL_OBJ_MANAGER("collapsing_props", TCompCollapsingProps)

void TCompCollapsingProps::load(const json& j, TEntityParseContext& ctx)
{
	spawn_prefab = j.value("spawn_prefab", "");
	assert(spawn_prefab.length());
	fall_time = j.value("fall_time", fall_time);
	type = j.value("type", "rocks");
	fmod_event = j.value("fmod_event", "ENV/Cave/Rocks_Falling");
	if(!fmod_event.length())
		fmod_event = "ENV/Cave/Rocks_Falling";
}

void TCompCollapsingProps::debugInMenu()
{
	if (ImGui::Button("Trigger"))
		onPlayerStepOn(TMsgEntityTriggerEnter());
}

/*
	Messages callbacks
*/

void TCompCollapsingProps::onPlayerStepOn(const TMsgEntityTriggerEnter& msg)
{
	CEntity* e_player = getEntityByName("player");
	CEntity* e_object_hit = e_player;// msg.h_entity;

	if (e_object_hit != e_player)
		return;

	TCompTransform* playerTransform = e_player->get<TCompTransform>();
	TCompTransform* transform = get<TCompTransform>();
	TEntityParseContext ctx;
	spawn(spawn_prefab, *transform, ctx);

	if (type == "rocks")
	{
		processRocks(ctx);
	}
	else if (type == "bridge")
	{
		processBridge(ctx);
	}
	else
	{
		assert(false && "Collapsing props should have a type!");
	}

	// delete this
	CHandle(this).getOwner().destroy();
	CHandleManager::destroyAllPendingObjects();
}

void TCompCollapsingProps::processBridge(TEntityParseContext& ctx)
{
	CEntity* e_player = getEntityByName("player");
	TCompTransform* playerTransform = e_player->get<TCompTransform>();
	TCompTransform* transform = get<TCompTransform>();

	for (auto& h : ctx.entities_loaded)
	{
		assert(h.isValid());
		CEntity* e_piece = h;

		TCompCollider* collider = e_piece->get<TCompCollider>();
		assert(collider);

		((physx::PxRigidDynamic*)collider->actor)->setLinearDamping(physx::PxReal(0.001f));
		((physx::PxRigidDynamic*)collider->actor)->setAngularDamping(physx::PxReal(0.001f));
		((physx::PxRigidDynamic*)collider->actor)->setMaxContactImpulse(0.3f);

		EngineLua.executeScript("destroyEntity('" + std::string(e_piece->getName()) + "')", Random::range(110.f, 130.f));
	}

	// Shake now and in "fall_time" seconds
	EngineLua.executeScript("shakeOnce(1, 0.2, 1)");
	EngineLua.executeScript("shakeOnce(3, 0.1, 2)", fall_time);

	// post fmod event
	EngineAudio.postEvent(fmod_event, transform->getPosition());
}

void TCompCollapsingProps::processRocks(TEntityParseContext& ctx)
{
	CEntity* e_player = getEntityByName("player");
	TCompTransform* playerTransform = e_player->get<TCompTransform>();
	TCompTransform* transform = get<TCompTransform>();

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

		((physx::PxRigidDynamic*)collider->actor)->setLinearDamping(physx::PxReal(0.05f));
		((physx::PxRigidDynamic*)collider->actor)->setAngularDamping(physx::PxReal(0.2f));
		((physx::PxRigidDynamic*)collider->actor)->setMaxContactImpulse(0.3f);

		TCompTransform* rockTransform = e_rock->get<TCompTransform>();
		float distToPlayer = rockTransform->distance(*playerTransform);
		rock->setWaitTime(powf(distToPlayer / maxDistance, 6.f) * 4.f);

		EngineLua.executeScript("destroyEntity('" + std::string(e_rock->getName()) + "')", Random::range(110.f, 130.f));
	}

	CEntity* outputCamera = getEntityByName("camera_mixed");
	TCompCameraShake* shaker = outputCamera->get<TCompCameraShake>();
	shaker->shakeOnce(1.5f, 0.25f, 0.1f);

	// call Lua to shake camera in "fall_time" seconds
	EngineLua.executeScript("shakeOnce(3, 0.1, 2)", fall_time);

	// post fmod event
	EngineAudio.postEvent("ENV/Cave/Rocks_Falling", transform->getPosition());
}