#include "mcv_platform.h"
#include "entity/entity.h"
#include "animation/animation_callback.h"
#include "components/cameras/comp_camera_follow.h"

struct turnCallback : public CAnimationCallback
{
	void AnimationUpdate(float anim_time, CalModel* model, CalCoreAnimation* animation, void* userData)
	{
		CEntity* camera = getEntityByName("camera_follow");
		assert(camera);
		if (!camera)
			return;

		TCompCameraFollow* followCam = camera->get<TCompCameraFollow>();
		assert(followCam);

		followCam->must_recenter = true;
	}

	void AnimationComplete(CalModel* model, CalCoreAnimation* animation, void* userData)
	{
		CEntity* camera = getEntityByName("camera_follow");
		assert(camera);
		if (!camera)
			return;

		TCompCameraFollow* followCam = camera->get<TCompCameraFollow>();
		assert(followCam);

		followCam->must_recenter = true;
	}
};

REGISTER_CALLBACK("onTurn", turnCallback)