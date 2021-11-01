#pragma once

#include "fsm/fsm_state.h"
#include "animation/blend_animation.h"

namespace fsm
{
    class CStateAnimation : public IState
    {
        TSimpleAnimation anim;

    public:
        void onEnter(CContext& ctx, const ITransition* transition = nullptr) const override;
        void onUpdate(CContext& ctx, float dt) const override;
        void onLoad(const json& params) override;
        void renderInMenu(CContext& ctx, const std::string& prefix = "") const override;
        bool hasAnimLoop() { return anim.isValid && anim.loop; }
    };
}