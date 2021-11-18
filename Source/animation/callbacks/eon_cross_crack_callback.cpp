#include "mcv_platform.h"
#include "engine.h"
#include "animation/animation_callback.h"
#include "components/audio/comp_music_interactor.h"
#include "audio/module_audio.h"


struct onEonCrossCrackCallback : public CAnimationCallback
{

	bool first = true;

	void AnimationUpdate(float anim_time, CalModel* model, CalCoreAnimation* animation, void* userData)
	{
		if (first) {
			first = false;
			CEntity* e_owner = getOwnerEntity(userData);
			EngineAudio.postEvent("CHA/Eon/CrossCrack", e_owner->getPosition());
		}
	}

	void AnimationComplete(CalModel* model, CalCoreAnimation* animation, void* userData)
	{
	}
};

REGISTER_CALLBACK("onEonCrossCrack", onEonCrossCrackCallback)