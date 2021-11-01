#include "mcv_platform.h"
#include "fsm/transitions/transition_wait_time.h"
#include "fsm/fsm_context.h"

namespace fsm
{
    bool CTransitionWaitTime::onCheck(CContext& ctx) const
    {
        return ctx.getTimeInState() >= maxTime;
    }

    void CTransitionWaitTime::onLoad(const json& params)
    {
        maxTime = params.value("time", maxTime);
        blend_time = params.value("blend_time", blend_time);
    }
}
