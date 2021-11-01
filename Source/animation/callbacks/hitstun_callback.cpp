#include "mcv_platform.h"
#include "entity/entity.h"
#include "engine.h"
#include "animation/animation_callback.h"
#include "components/common/comp_fsm.h"
#include "fmod_studio.hpp"

struct onHitstunCallback : public CAnimationCallback
{
	bool callback_called = false;

	void AnimationUpdate(float anim_time, CalModel* model, CalCoreAnimation* animation, void* userData)
	{
		if (!callback_called) {
			callback_called = true;
			CEntity* e_owner = getOwnerEntity(userData);
			TCompFSM* c_fsm = e_owner->get<TCompFSM>();
			if (!c_fsm)
				return;

			if(c_fsm->getCtx()._cur_fmod_event)
				c_fsm->getCtx()._cur_fmod_event->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
		}
	}

	void AnimationComplete(CalModel* model, CalCoreAnimation* animation, void* userData) { callback_called = false; }
};

REGISTER_CALLBACK("onHitstun", onHitstunCallback)