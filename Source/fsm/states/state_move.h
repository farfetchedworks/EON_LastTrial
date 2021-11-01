#pragma once

#include "fsm/fsm_state.h"

namespace fsm
{
    class CStateMove : public IState
    {
    public:
        void onUpdate(CContext& ctx, float dt) const override;
        void onLoad(const json& params) override;

        VEC3 offset = VEC3::Zero;
        float duration = 0.f;
    };
}