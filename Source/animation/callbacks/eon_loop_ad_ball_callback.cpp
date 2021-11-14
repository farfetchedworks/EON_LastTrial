#include "mcv_platform.h"
#include "engine.h"
#include "animation/animation_callback.h"
#include "components/audio/comp_music_interactor.h"
#include "audio/module_audio.h"
#include "fmod_studio.hpp"


struct onEonLoopADBallCallback : public CAnimationCallback
{
	bool first = true;
	FMOD::Studio::EventInstance* fmod_ev = nullptr;

	void AnimationUpdate(float anim_time, CalModel* model, CalCoreAnimation* animation, void* userData)
	{
		if (first) {
			first = false;
			CEntity* e_owner = getOwnerEntity(userData);
			fmod_ev = EngineAudio.postEventGetInst("CHA/Eon/AT/Eon_Loop_AD_Ball", e_owner->getPosition());
		}
	}

	void AnimationComplete(CalModel* model, CalCoreAnimation* animation, void* userData)
	{
		first = true;
		fmod_ev->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
	}
};

REGISTER_CALLBACK("onEonLoopADBall", onEonLoopADBallCallback)