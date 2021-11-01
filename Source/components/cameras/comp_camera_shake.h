#pragma once
#include "components/common/comp_base.h"
#include "entity/entity.h"

class TCompCameraShake : public TCompBase {

	DECL_SIBLING_ACCESS();

	// 3D Shaking
	VEC3 shakePos;
	CHandle hListener;
	std::string listener_name;
	float listenerRadius = 1.f;
	// 

	VEC3 shakeOffset;
	bool sustain = false;
	float currentFadeTime = 1.f;
	float fadeInDuration = 0.0f;
	float fadeOutDuration = 0.f;
	float Magnitude = 0.f;

	bool IsShaking() { return currentFadeTime > 0 || sustain; }
	bool IsFadingOut() { return !sustain && currentFadeTime > 0; }
	bool IsFadingIn() { return currentFadeTime < 1.f && sustain && fadeInDuration > 0; }

public:

	void load(const json& j, TEntityParseContext& ctx);
	void update(float dt);

	void shake(float amount, float fadeIn);
	void shakeOnce(float amount, float fadeIn, float fadeOut);
	void stop(float fadeOut);

	void setListener(CHandle listener) { hListener = listener; };
};