#pragma once

#include "fsm/states/anim/state_animation.h"

namespace fsm
{
    class CStateBlendAnimation : public CStateAnimation
    {
        TBlendAnimation anim;

    public:
        void onEnter(CContext& ctx, const ITransition* transition = nullptr) const override;
        void onUpdate(CContext& ctx, float dt) const override;
        void onLoad(const json& params) override;
        void renderInMenu(CContext& ctx, const std::string& prefix = "") const override;
    };
}