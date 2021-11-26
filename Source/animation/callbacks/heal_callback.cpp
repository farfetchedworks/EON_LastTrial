#include "mcv_platform.h"
#include "entity/entity.h"
#include "engine.h"
#include "animation/animation_callback.h"
#include "components/messages.h"
#include "components/abilities/comp_heal.h"
#include "components/common/comp_fsm.h"

struct onHealCallback : public CAnimationCallback
{
	void AnimationUpdate(float anim_time, CalModel* model, CalCoreAnimation* animation, void* userData)
	{
		
	}

	void AnimationComplete(CalModel* model, CalCoreAnimation* animation, void* userData)
	{
		CEntity* owner = getOwnerEntity(userData);
		TCompFSM* c_fsm = owner->get<TCompFSM>();
		assert(c_fsm);
		if (!c_fsm)
			return;

		fsm::CContext ctx = c_fsm->getCtx();
		bool stunned = std::get<bool>(ctx.getVariableValue("is_stunned")) || std::get<bool>(ctx.getVariableValue("is_death"));

		if (stunned)
			return;

		TCompHeal* heal_comp = owner->get<TCompHeal>();
		assert(heal_comp);
		heal_comp->heal();
	}
};

REGISTER_CALLBACK("onHeal", onHealCallback)