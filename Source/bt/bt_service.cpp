#pragma once

#include "mcv_platform.h"
#include "bt_common.h"
#include "bt/bt_service.h"
#include "bt/bt_context.h"

EBTNodeResult IBTService::tickService(CBTContext& ctx, float dt)
{
	float period_accum = ctx.getNodeVariable<float>(name, "period_accum");
	period_accum += dt;

	if (period_accum >= period) {
		EBTNodeResult res = executeService(ctx, dt);

		// !! WARNING !! : period_acum should be decremented AFTER executing the service,
		// as some services (e.g.: Cooldown) access the period elapsed
		period_accum -= period;
		ctx.setNodeVariable(name, "period_accum", period_accum);
		return res;
	}
	else {
		ctx.setNodeVariable(name, "period_accum", period_accum);
		return EBTNodeResult::SUCCEEDED;
	}
}

// Set the period accumulated to 0 only if it is not valid
void IBTService::onEnter(CBTContext& ctx) {
	if (!ctx.isNodeVariableValid(name, "period_accum")) {
		ctx.setNodeVariable(name, "period_accum", 0.f);
	}
}