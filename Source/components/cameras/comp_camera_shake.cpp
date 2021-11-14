#include "mcv_platform.h"
#include "comp_camera_shake.h"
#include "components/common/comp_camera.h"
#include "components/common/comp_transform.h"

DECL_OBJ_MANAGER("camera_shake", TCompCameraShake)

void TCompCameraShake::load(const json& j, TEntityParseContext& ctx)
{
	listener_name = j.value("listener", "");
	listenerRadius = j.value("radius", listenerRadius);
}

void TCompCameraShake::update(float dt)
{
	TCompCamera* camera = get<TCompCamera>();
	TCompTransform* transform = get<TCompTransform>();

	VEC3 pos = transform->getPosition();
	VEC3 target = pos + transform->getForward();

	float shakeScale = Magnitude;

	if (!hListener.isValid() && listener_name.size())
		hListener = getEntityByName(listener_name);

	// Modulate Magnitude using Listener's position
	if (use3D && hListener.isValid()) {
		CEntity* eListener = hListener;
		float distance = VEC3::Distance(shakePos, eListener->getPosition());
		shakeScale *= (1.f - clampf((distance / listenerRadius), 0.f, 1.f));
	}

	if (IsShaking() || IsFadingIn() || IsFadingOut())
	{
		shakeOffset = Random::vec3();
		shakeOffset = (shakeOffset * 2.f) - VEC3(1.f);

		if (fadeInDuration > 0 && sustain)
		{
			if (currentFadeTime < 1.f)
				currentFadeTime += dt / fadeInDuration;
			else if (fadeOutDuration > 0)
				sustain = false;
		}

		if (!sustain)
			currentFadeTime -= dt / fadeOutDuration;

		shakeOffset *= shakeScale * currentFadeTime * dt;

		pos.x += shakeOffset.x;
		pos.y += shakeOffset.y;
	}

	camera->lookAt(pos, target);
}

void TCompCameraShake::shake(float amount, float fadeIn, bool is_3d)
{
	assert(fadeIn > 0.f);
	TCompTransform* transform = get<TCompTransform>();
	shakePos = transform->getPosition();
	use3D = is_3d;

	Magnitude = amount;
	fadeInDuration = fadeIn;
	fadeOutDuration = 0.f;

	if (fadeInDuration > 0)
	{
		sustain = true;
		currentFadeTime = 0;
	}
	else
	{
		sustain = false;
		currentFadeTime = 1.f;
	}
}

void TCompCameraShake::shakeOnce(float amount, float fadeIn, float fadeOut, bool is_3d)
{
	shake(amount, fadeIn, is_3d);
	fadeOutDuration = fadeOut;
}

void TCompCameraShake::stop(float fadeOut)
{
	fadeOutDuration = fadeOut;
}