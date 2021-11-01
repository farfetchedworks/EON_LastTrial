#include "mcv_platform.h"
#include "fsm/transitions/transition_wait_state_finished.h"
#include "fsm/fsm_context.h"

namespace fsm
{
    bool CTransitionWaitStateFinished::onCheck(CContext& ctx) const
    {
        return ctx.isStateFinished();
    }
}
