#pragma once

#include "mcv_platform.h"
#include "ui/ui.fwd.h"

namespace ui
{
    class CEffect
    {
    public:
        virtual std::string_view getType() const { return "Effect"; }
        virtual void update(CWidget* widget, float elapsed) {};
        virtual void renderInMenu() {};
    };
}
