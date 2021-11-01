#pragma once

#include "fsm/fsm_state.h"
#include "fsm/fsm_context.h"
#include "animation/blend_animation.h"

namespace fsm
{
	enum class EStateCancel {
		NOT_CANCELLABLE,
		ON_HIT,			// Cancellable if entity receives a hit
		ON_DECISION,	// Cancellable if entity decides so
		ON_ANY			// = ON_HIT or ON_DECISION
	};

	struct SActionTimings {

		float startup = 0.0f;
		float active = 0.0f;
		float recovery = 0.0f;

		float getTime() const { return startup + active + recovery; }

		void fill(const VEC3& v) {
			startup = v.x;
			active = v.y;
			recovery = v.z;
		}
	};

	auto const null_logic = [](CContext&) {};

	struct SLogicCallbacks {
		std::function<void(CContext&)> onStartup			= null_logic;
		std::function<void(CContext&)> onStartupFinished	= null_logic;
		std::function<void(CContext&)> onActive				= null_logic;
		std::function<void(CContext&)> onActiveFinished		= null_logic;
		std::function<void(CContext&)> onRecovery			= null_logic;
		std::function<void(CContext&)> onRecoveryFinished	= null_logic;
	};

    class CStateBaseLogic : public IState
    {
		bool use_timings = false;
		bool loadStateTimings(const json& params);

    protected:

		TSimpleAnimation	anim;
		SActionTimings		timings;
		SLogicCallbacks		callbacks;
		EStateCancel		cancel_mode = EStateCancel::NOT_CANCELLABLE;

		// Load timings, cancel mode and other info from state's json
		void loadStateProperties(const json& params);

		bool inStartup(float t) const { return t >= 0 && t < timings.startup; }
		bool inActive(float t) const { return t >= timings.startup && t < (timings.startup + timings.active); }
		bool inRecovery(float t) const { return t >= (timings.startup + timings.active) && t < (timings.startup + timings.active + timings.recovery); }
		bool isFinished(float t) const { return t >= (timings.startup + timings.active + timings.recovery); };

		void updateLogic(CContext& ctx, float dt) const
		{
			float time = ctx.getTimeInState();

			if (inStartup(time)) callbacks.onStartup(ctx);
			else if (inActive(time)) {
				if (inStartup(time - dt)) callbacks.onStartupFinished(ctx);
				callbacks.onActive(ctx);
			} else if (inRecovery(time)) {
				if (inActive(time - dt)) callbacks.onActiveFinished(ctx);
				callbacks.onRecovery(ctx);
			} else if (isFinished(time)) {
				callbacks.onRecoveryFinished(ctx);
			}
		}

    public:
        void renderInMenu(CContext& ctx, const std::string& prefix = "") const override;

		bool inStartupFrames(CContext& ctx) const { return inStartup(ctx.getTimeInState()); };
		bool inActiveFrames(CContext& ctx) const { return inActive(ctx.getTimeInState()); };
		bool inRecoveryFrames(CContext& ctx) const { return inRecovery(ctx.getTimeInState()); };
		
		bool isCancellableOnHit() { return (cancel_mode == EStateCancel::ON_HIT) || (cancel_mode == EStateCancel::ON_ANY); }
		bool isCancellableOnDecision() { return (cancel_mode == EStateCancel::ON_DECISION) || (cancel_mode == EStateCancel::ON_ANY); }

		bool hasValidLoop() { return anim.isValid && anim.loop; }
        TSimpleAnimation& getAnimation() { return anim; };

		bool checkContext(CContext& ctx) const override;

		bool usesTimings() const { return use_timings; };
    };
}