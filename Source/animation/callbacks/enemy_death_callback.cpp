#include "mcv_platform.h"
#include "entity/entity.h"
#include "entity/entity_parser.h"
#include "engine.h"
#include "lua/module_scripting.h"
#include "animation/animation_callback.h"
#include "components/cameras/comp_camera_shake.h"
#include "components/common/comp_transform.h"
#include "components/stats/comp_health.h"
#include "components/common/comp_render.h"
#include "components/render/comp_dissolve.h"
#include "components/particles/comp_follow_target.h"
#include "components/stats/comp_geons_drop.h"
#include "components/controllers/pawn_utils.h"

struct onEnemyDeathCallback : public CAnimationCallback
{
	void AnimationUpdate(float anim_time, CalModel* model, CalCoreAnimation* animation, void* userData)
	{
		
	}

	void AnimationComplete(CalModel* model, CalCoreAnimation* animation, void* userData)
	{
		CEntity * e_owner = getOwnerEntity(userData);
		TCompHealth* c_health = e_owner->get<TCompHealth>();

		// if the same enemy completed its animation before, don't do anything more
		// in other case, this sets completion to true inside
		if (c_health->checkDeathAnimationCompleted())
			return;

		PawnUtils::spawnGeons(e_owner->getPosition(), e_owner);

		std::string name = e_owner->getName();
		std::string argument = "destroyEntity('" + name + "')";

		// sync with dissolve time
		EngineLua.executeScript(argument, 20.f);
	}
};

REGISTER_CALLBACK("onEnemyDeath", onEnemyDeathCallback)