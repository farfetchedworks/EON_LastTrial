#include "mcv_platform.h"
#include "entity/entity.h"
#include "engine.h"
#include "animation/animation_callback.h"
#include "modules/game/module_player_interaction.h"
#include "components/messages.h"
#include "components/gameplay/comp_launch_animation.h"
#include "components/controllers/comp_player_controller.h"

struct onLaunchAnimationCallback : public CAnimationCallback
{
	void AnimationUpdate(float anim_time, CalModel* model, CalCoreAnimation* animation, void* userData)
	{
		
	}

	void AnimationComplete(CalModel* model, CalCoreAnimation* animation, void* userData)
	{
		CEntity* owner = getOwnerEntity(userData);
		assert(owner);

		TCompLaunchAnimation* c_animation = PlayerInteraction.getAnimationLauncher();
		if(c_animation)
			c_animation->oncomplete();
	}
};

REGISTER_CALLBACK("onLaunchAnimation", onLaunchAnimationCallback)