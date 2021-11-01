#pragma once

#include "mcv_platform.h"
#include "fsm.fwd.h"

namespace fsm
{
    class ITransition;

    class IState
    {
    public:
        virtual void onEnter(CContext& ctx, const ITransition* transition = nullptr) const { printf("OnEnter %s", name.c_str()); }
        virtual void onUpdate(CContext& ctx, float dt) const {}
        virtual void onExit(CContext& ctx) const { printf("OnExit %s", name.c_str()); }
        virtual void onLoad(const json& params) {}
        virtual void renderInMenu(CContext& ctx, const std::string& prefix = "") const;
        virtual void renderInMenu(const std::string& prefix = "") const;

        virtual bool checkContext(CContext& ctx) const { return true; }

        bool isBlendSpace() { return _blendState; }
        void setAsBlendState() { _blendState = true; }

        std::string_view type;
        std::string name;
        VTransitions transitions;

        bool _blendState = false;
    };

    using CStateDummy = IState;
}
