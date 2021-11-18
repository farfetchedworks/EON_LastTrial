#include "mcv_platform.h"
#include "event_callback.h"
#include "engine.h"
#include "modules/module_events.h"
#include "audio/module_audio.h"
#include "fsm/fsm.fwd.h"
#include "components/common/comp_fsm.h"
#include "fsm/states/logic/state_locomotion.h"
#include "animation/blend_animation.h"

void EventCallback::parse(const json& j)
{
	name = j["name"];
	loop = j.value("loop", loop);
	restrict_mw = j.value("restrict_mw", restrict_mw);

	// ------------
	// Read type
	std::string type = j.value("type", "default");
	if (type == "audio") {
		this->type = EType::AUDIO;
	}

	// ------------
	// Read timestamps
	if (j.count("timestamps")) {

		for (int i = 0; i < j["timestamps"].size(); ++i) {

			float frame = j["timestamps"][i];

			ETimeStamp ts;
			ts.mark = (float)frame / 30.f;
			timestamps.push_back(ts);
		}
	}
}

void EventCallback::AnimationUpdate(float anim_time, CalModel* model, CalCoreAnimation* animation, void* userData)
{
	if (!timestamps.size())
		return;

	CEntity* owner = getOwnerEntity(userData);
	assert(owner);

	TCompFSM* fsm = owner->get<TCompFSM>();
	bool skip_audio = false;

	if (fsm) {
		fsm::CContext& ctx = fsm->getCtx();
		fsm::CStateBaseLogic* current_state = (fsm::CStateBaseLogic*)ctx.getCurrentState();

		// only fire event for the maximum weight animation
		if (current_state->isBlendSpace()) {
			auto blendstate = (CStateLocomotion*)current_state;
			TBlendAnimation& anim = blendstate->getAnimation();
			const std::string& anim_name = anim.getMaxWeightAnimation(ctx);
			skip_audio |= (!anim_name.size() || animation->getName() != anim_name);

			if (skip_audio && restrict_mw)
				return;
		}
	}

	float time_threshold = 0.04f;

	if (loop && anim_time >= (animation->getDuration() - time_threshold)) {
		AnimationComplete(model, animation, userData);
		return;
	}

	for (auto& ts : timestamps) {

		if (anim_time < ts.mark || ts.fired)
			continue;

		if (type == EType::AUDIO) {
			EngineAudio.postEvent(name, owner);
		}
		else {
			EventSystem.dispatchEvent(name, owner);
		}

		ts.fired = true;
	}
}

void EventCallback::AnimationComplete(CalModel* model, CalCoreAnimation* animation, void* userData)
{
	// Don't reset timestamps in locked animations
	if (getState(model, animation) == CalAnimation::STATE_STOPPED)
		return;

	for (auto& ts : timestamps)
		ts.fired = false;
}

REGISTER_CALLBACK("event", EventCallback)