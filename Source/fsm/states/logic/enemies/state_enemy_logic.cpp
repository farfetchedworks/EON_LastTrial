#include "mcv_platform.h"
#include "state_enemy_logic.h"

namespace fsm
{
	void CStateEnemyLogic::onFirstFrameLogic(CContext& ctx) const
	{
		ctx.setVariableValue("pend_first_frame", true);
	}

	void CStateEnemyLogic::onLoad(const json& params)
	{
		anim.load(params);
		loadStateProperties(params);
		control_variable_name = params["control_var"];

		callbacks.onStartup = [&](CContext& ctx)
		{
			ctx.setVariableValue("pend_startup", true);
		};

		callbacks.onStartupFinished = [&](CContext& ctx)
		{
			ctx.setVariableValue("pend_startup_finished", true);
		};

		callbacks.onActive = [&](CContext& ctx)
		{
			ctx.setVariableValue("pend_active", true);
		};

		callbacks.onActiveFinished = [&](CContext& ctx)
		{
			ctx.setVariableValue("pend_active_finished", true);
		};

		callbacks.onRecovery = [&](CContext& ctx)
		{
			ctx.setVariableValue("pend_recovery", true);
		};

		callbacks.onRecoveryFinished = [&](CContext& ctx)
		{
			// ctx.setVariableValue("pend_recovery_finished", true);
			// Unnecessary, it must be called from task tickCondition()
		};
	}

	void CStateEnemyLogic::onEnter(CContext& ctx, const ITransition* transition) const
	{
		anim.play(ctx, transition);
		onFirstFrameLogic(ctx);
	}

	void CStateEnemyLogic::onExit(CContext& ctx) const
	{
		// To avoid resetting variables that should be greater than 2 (such as regular attacks) when a combo is active
		// Also, the variable name must not contain the word "phase" because phase numbers must not be
		if(!std::get<bool>(ctx.getVariableValue("combo_attack_active")) && control_variable_name.find("phase") == std::string::npos)
			ctx.setVariableValue(control_variable_name, 0);
		
		anim.stop(ctx);
	}

	void CStateEnemyLogic::onUpdate(CContext& ctx, float dt) const
	{
		anim.update(ctx, dt);
		updateLogic(ctx, dt);
	}
};
