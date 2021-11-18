#include "mcv_platform.h"
#include "entity/entity.h"
#include "engine.h"
#include "animation/animation_callback.h"
#include "components/messages.h"
#include "components/common/comp_transform.h"
#include "bt/task_utils.h"
#include "components/ai/comp_bt.h"
#include "audio/module_audio.h"
#include "components/stats/comp_health.h"
#include "modules/module_boot.h"
#include "lua/module_scripting.h"

struct onCygnusF2ToF3 : public CAnimationCallback
{
	bool first_update = true;

	void AnimationUpdate(float anim_time, CalModel* model, CalCoreAnimation* animation, void* userData)
	{
		if (first_update) {
			first_update = false;

			// Intro form 3
			EngineLua.executeScript("CinematicCygnusF2ToF3()");

			EngineAudio.setMusicRTPC("Cygnus_Phase", 3, true);
			// EngineAudio.postEvent("CHA/Cygnus/P1/AT/Cygnus_P1_To_P2", CHandle(getOwnerEntity(userData)));
		}
	}

	void AnimationComplete(CalModel* model, CalCoreAnimation* animation, void* userData)
	{
		if (!Boot.inGame())
			return;

		// Enable BT
		CEntity* e_owner = getOwnerEntity(userData);
		TCompBT* c_bt = e_owner->get<TCompBT>();
		assert(c_bt);
		c_bt->setEnabled(true);

		// Show health bar
		TCompHealth* c_health = e_owner->get<TCompHealth>();
		c_health->setRenderActive(true);
	}
};

REGISTER_CALLBACK("onCygnusF2ToF3", onCygnusF2ToF3)