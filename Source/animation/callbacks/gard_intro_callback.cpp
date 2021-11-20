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
#include "components/common/comp_collider.h"
#include "components/common/comp_parent.h"
#include "components/common/comp_tags.h"
#include "components/common/comp_hierarchy.h"
#include "skeleton/comp_skeleton.h"

struct onGardIntro : public CAnimationCallback
{
	void AnimationUpdate(float anim_time, CalModel* model, CalCoreAnimation* animation, void* userData)
	{
		
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
		c_health->setHealth(c_health->getMaxHealth());
		c_health->setRenderActive(true, "gard");

		CEntity* player = getEntityByName("player");
		TCompCollider* c_collider = player->get<TCompCollider>();
		c_collider->enableBoxController();

		VHandles lianas = CTagsManager::get().getAllEntitiesByTag("blocking_lianas");
		for (auto h : lianas) {
			assert(h.isValid());
			CEntity* e = h;
			TCompSkeleton* comp_skel = e->get<TCompSkeleton>();
			comp_skel->resume(0.8f);
		}
	}
};

REGISTER_CALLBACK("onGardIntro", onGardIntro)