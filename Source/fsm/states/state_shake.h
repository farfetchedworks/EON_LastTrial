#pragma once

#include "fsm/fsm_state.h"

namespace fsm
{
    class CStateShake : public IState
    {
    public:
        void onLoad(const json& params) override;
        void onEnter(CContext& ctx, const ITransition* transition = nullptr) const override;
        void onExit(CContext& ctx) const override;

        float amount = 0.f;
    };
}