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

struct onCygnusF1ToF2 : public CAnimationCallback
{
	float t = 0.f;
	bool correct_pos = false;
	bool first_update = true;

	void AnimationUpdate(float anim_time, CalModel* model, CalCoreAnimation* animation, void* userData)
	{
		if (first_update) {
			first_update = false;
			EngineAudio.postEvent("CHA/Cygnus/P1/AT/Cygnus_P1_To_P2", CHandle(getOwnerEntity(userData)));
		}

		// Nothing [0-4.35]
		if (anim_time < 4.35f)
			return;

		CEntity* bhole = getEntityByName("Cygnus_Black_Hole");
		if (!bhole)
			return;

		TCompTransform* trans = bhole->get<TCompTransform>();

		// Open [4.35-5.25]
		if (anim_time < 5.25f)
		{
			if (!correct_pos) {
				CEntity* e = getOwnerEntity(userData);
				VEC3 pos = TaskUtils::getBoneWorldPosition(e, "cygnus_hole_jnt");
				trans->setPosition(pos);
				correct_pos = true;
			}

			t += Time.delta * 0.8f;
			trans->setScale(VEC3(t));
		}
		// Close [9.25-end]
		else if (anim_time > 9.f) {
			t -= Time.delta * 0.8f;
			t = std::max(t, 0.f);
			trans->setScale(VEC3(t));
		}
	}

	void AnimationComplete(CalModel* model, CalCoreAnimation* animation, void* userData)
	{
		// Enable BT
		CEntity* e_owner = getOwnerEntity(userData);
		TCompBT* c_bt = e_owner->get<TCompBT>();
		assert(c_bt);
		c_bt->setEnabled(true);

		// Show health bar
		TCompHealth* c_health = e_owner->get<TCompHealth>();
		c_health->setRenderActive(true);

		// Destroy Black Hole
		CEntity* bhole = getEntityByName("Cygnus_Black_Hole");
		if (!bhole)
			return;

		bhole->destroy();
	}
};

REGISTER_CALLBACK("onCygnusF1ToF2", onCygnusF1ToF2)