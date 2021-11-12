#pragma once

#include "ui/ui_effect.h"

namespace ui
{
    class CEffect_FadeIn : public CEffect
    {
    public:
        std::string_view getType() const override { return "Fade In"; }
        void update(CWidget* widget, float elapsed) override;
        void renderInMenu() override;

        float getFadeTime() { return _time; }
        void setFadeTime(float time) { _time = time; }
        void setTimeFactor(CWidget* widget, float f);
        virtual void setTime(CWidget* widget);

    protected:
        float _time = 1.f;
        float _timer = 0.f;
        float _factor = 0.f;
    };

    class CEffect_FadeOut : public CEffect_FadeIn
    {
    public:
        std::string_view getType() const override { return "Fade Out"; }
        void update(CWidget* widget, float elapsed) override;
        void setTime(CWidget* widget) override;
    };
}
