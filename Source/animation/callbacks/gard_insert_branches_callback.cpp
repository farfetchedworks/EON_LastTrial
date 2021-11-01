#include "mcv_platform.h"
#include "entity/entity.h"
#include "engine.h"
#include "animation/animation_callback.h"
#include "components/messages.h"
#include "components/cameras/comp_camera_shake.h"

struct onGardInsertBranchesCallback : public CAnimationCallback
{
	// Right now its manually adjusted
	const float INSERT_BRANCHES_TIMESTAMP = 0.9f;
	bool callback_called = false;

	void AnimationUpdate(float anim_time, CalModel* model, CalCoreAnimation* animation, void* userData)
	{
		if (!callback_called && anim_time >= INSERT_BRANCHES_TIMESTAMP) {
			callback_called = true;
			CEntity* outputCamera = getEntityByName("camera_mixed");
			TCompCameraShake* shaker = outputCamera->get<TCompCameraShake>();
			shaker->shakeOnce(5.f, 0.05f, 0.75f);
		}
	}

	void AnimationComplete(CalModel* model, CalCoreAnimation* animation, void* userData)
	{
	}
};

REGISTER_CALLBACK("onGardInsertBranches", onGardInsertBranchesCallback)