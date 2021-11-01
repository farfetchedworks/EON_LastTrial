#pragma once

#include "mcv_platform.h"
#include "fsm.fwd.h"

namespace fsm
{
    class ITransition
    {
    public:
        virtual bool onCheck(CContext& ctx) const { return false; }
        virtual void onLoad(const json& params) {}
        virtual void renderInMenu() const;

        float blend_time = 0.f;

        const IState* source = nullptr;
        const IState* target = nullptr;
        
        std::string_view type;
    };
}
