#pragma once

#include "fsm/fsm_transition.h"

namespace fsm
{
    class CTransitionWaitTime : public ITransition
    {
    public:
        bool onCheck(CContext& ctx) const override;

        void onLoad(const json& params) override;

        float maxTime = 0.f;
    };
}