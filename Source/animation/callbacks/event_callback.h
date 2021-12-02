#pragma once
#include "entity/entity.h"
#include "animation/animation_callback.h"
#include "components/messages.h"

struct EventCallback : public CAnimationCallback
{
	enum class EType {
		DEFAULT,
		SCRIPT,
		AUDIO
	};

	struct ETimeStamp {
		float mark = 0.f;
		bool fired = false;
	};

	std::string name	= "";
	EType type			= EType::DEFAULT;
	bool loop			= false;
	bool restrict_mw	= true;

	std::vector<ETimeStamp> timestamps;

	void parse(const json& j);

	void AnimationUpdate(float anim_time, CalModel* model, CalCoreAnimation* animation, void* userData);
	void AnimationComplete(CalModel* model, CalCoreAnimation* animation, void* userData);
};