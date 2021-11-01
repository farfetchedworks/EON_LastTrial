#include "mcv_platform.h"
#include "entity/entity.h"
#include "entity/entity_parser.h"
#include "engine.h"
#include "lua/module_scripting.h"
#include "animation/animation_callback.h"
#include "components/common/comp_transform.h"
#include "components/controllers/pawn_utils.h"
#include "components/projectiles/comp_gard_branch.h"

struct onGardBranchFinished : public CAnimationCallback
{
	void AnimationUpdate(float anim_time, CalModel* model, CalCoreAnimation* animation, void* userData)
	{
		if (anim_time >= (animation->getDuration() - 0.05f)) {
			AnimationComplete(model, animation, userData);
		}
	}

	void AnimationComplete(CalModel* model, CalCoreAnimation* animation, void* userData)
	{
		CEntity * e_owner = getOwnerEntity(userData);
		if (!e_owner)
			return;

		TCompGardBranch* branch = e_owner->get<TCompGardBranch>();
		if (branch)
			branch->destroy();
		else
			e_owner->destroy();
	}
};

REGISTER_CALLBACK("onBranchFinished", onGardBranchFinished)