#pragma once

#include "fsm/fsm_transition.h"

namespace fsm
{
    class CTransitionWaitStateFinished : public ITransition
    {
    public:
        bool onCheck(CContext& ctx) const override;
    };
}