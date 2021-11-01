#include "mcv_platform.h"
#include "entity/entity.h"
#include "engine.h"
#include "animation/animation_callback.h"
#include "components/messages.h"
#include "components/gameplay/comp_game_manager.h"

struct onDeathCallback : public CAnimationCallback
{
	void AnimationUpdate(float anim_time, CalModel* model, CalCoreAnimation* animation, void* userData)
	{
		
	}

	void AnimationComplete(CalModel* model, CalCoreAnimation* animation, void* userData)
	{
		TCompGameManager* comp_gm = GameManager->get<TCompGameManager>();
		comp_gm->setEonDied();
	}
};

REGISTER_CALLBACK("onDeath", onDeathCallback)