#include "mcv_platform.h"
#include "entity/entity.h"
#include "engine.h"
#include "modules/module_boot.h"
#include "animation/animation_callback.h"
#include "components/cameras/comp_camera_shake.h"
#include "components/gameplay/comp_game_manager.h"
#include "components/stats/comp_health.h"
#include "components/render/comp_dissolve.h"
#include "components/controllers/pawn_utils.h"
#include "components/common/comp_collider.h"
#include "components/common/comp_tags.h"
#include "skeleton/comp_skeleton.h"
#include "lua/module_scripting.h"
#include "ui/ui_module.h"
#include "ui/widgets/ui_text.h"
#include "bt/task_utils.h"
#include "ui/ui_module.h"
#include "audio/module_audio.h"

const float DEATH_TIME = 30.f;

struct onGardDeathCallback : public CAnimationCallback
{

	bool first_update = true;

	void AnimationUpdate(float anim_time, CalModel* model, CalCoreAnimation* animation, void* userData)
	{
		if (first_update) {
			first_update = false;
			EngineAudio.setGlobalRTPC("Gard_Phase", 4);
		}
	}

	void AnimationComplete(CalModel* model, CalCoreAnimation* animation, void* userData)
	{
		CEntity* e_owner = getOwnerEntity(userData);
		TCompHealth* c_health = e_owner->get<TCompHealth>();
		if (!c_health)
			return;

		// if the same enemy completed its animation before, don't do anything more
		// in other case, this sets completion to true inside
		if (c_health->checkDeathAnimationCompleted())
			return;

		c_health->setRenderActive(false);

		// Shake camera
		{
			CEntity* outputCamera = getEntityByName("camera_mixed");
			TCompCameraShake* shaker = outputCamera->get<TCompCameraShake>();
			shaker->shakeOnce(1.2f, 0.01f, 0.35f);
		}
		
		// Destroy entity
		{
			EngineLua.executeScript("destroyEntity('Gard')", DEATH_TIME);
			CEntity* e_game_manager = getEntityByName("game_manager");
			e_game_manager->sendMsg(TMsgBossDead({ "Gard" }));
		}

		// Remove lianas
		{
			VHandles lianas = CTagsManager::get().getAllEntitiesByTag("blocking_lianas");
			for (auto h : lianas) {
				assert(h.isValid());
				CEntity* e = h;
				TCompSkeleton* comp_skel = e->get<TCompSkeleton>();
				comp_skel->resume();
			}
		}

		// Spawn floor particles and geons
		{
			VEC3 spawnPos = e_owner->getPosition() + e_owner->getForward() * 3;

			// TODO PABLO LG: POner humo en lugar de roquitas
			spawnParticles("data/particles/gard_floor_particles.json", spawnPos, spawnPos, 2.f);
			PawnUtils::spawnGeons(spawnPos, e_owner);
		}

		{
			CEntity* player = getEntityByName("player");
			TCompCollider* c_collider = player->get<TCompCollider>();
			c_collider->enableCapsuleController();
		}

		// Test progressive loading: "data/scenes/templelevel.json"
		/*{
			bool is_ok = Boot.loadScene("data/scenes/templelevel.json");
			assert(is_ok);
		}*/
	}
};

REGISTER_CALLBACK("onGardDeath", onGardDeathCallback)