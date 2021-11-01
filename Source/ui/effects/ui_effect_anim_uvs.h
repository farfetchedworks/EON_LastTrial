#pragma once

#include "ui/ui_effect.h"

namespace ui
{
    class CEffect_AnimateUV : public CEffect
    {
    public:
        std::string_view getType() const override { return "Animate UVs"; }
        void update(CWidget* widget, float elapsed) override;
        void renderInMenu() override;

        void setSpeed(const VEC2& speed) { _speed = speed; }

    protected:
        VEC2 _speed = VEC2::Zero;
    };
}
