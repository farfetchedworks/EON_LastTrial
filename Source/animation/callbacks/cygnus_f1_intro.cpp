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
#include "components/common/comp_tags.h"
#include "components/common/comp_parent.h"
#include "components/common/comp_hierarchy.h"

struct onCygnusF1Intro : public CAnimationCallback
{
	void AnimationUpdate(float anim_time, CalModel* model, CalCoreAnimation* animation, void* userData)
	{
		if (anim_time < 1.5f)
			return;

		CEntity* e = nullptr;

		// Cygnus shrine position
		{
			VHandles cShrineTags = CTagsManager::get().getAllEntitiesByTag(getID("SmegShrine"));
			if(cShrineTags.size() != 1)
				return;
			e = cShrineTags[0];
		}
		
		if (!e)
			return;

		TCompParent* parent = e->get<TCompParent>();
		e = parent->getChildByName("shrine_black_hole");
		TCompHierarchy* trans = e->get<TCompHierarchy>();

		if (anim_time < 10.f)
		{
			trans->setScale(damp<VEC3>(trans->getScale(), VEC3(1.6f), 1.f, Time.delta));
		}
		else {
			trans->setScale(damp<VEC3>(trans->getScale(), VEC3(0.9f), 1.f, Time.delta));
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
		c_health->setRenderActive(true, "cygnus");
	}
};

REGISTER_CALLBACK("onCygnusIntro", onCygnusF1Intro)