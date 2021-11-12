#include "mcv_platform.h"
#include "ui_effect_fade.h"
#include "ui/widgets/ui_image.h"

namespace ui
{
    void CEffect_FadeIn::setTime(CWidget* widget)
    {
        setTimeFactor(widget, clampf(_timer / _time, 0.f, 1.f));
    }

    void CEffect_FadeIn::setTimeFactor(CWidget* widget, float f)
    {
        _factor = f;

        TImageParams* iParams = widget->getImageParams();
        if (iParams)
            iParams->time_normalized = f;
        else
        {
            TVideoParams* vParams = widget->getVideoParams();
            if (vParams)
                vParams->time_normalized = f;
        }
    }

    void CEffect_FadeIn::update(CWidget* widget, float elapsed)
    {
        if (widget->getState() != EState::STATE_IN)
            return;

        _timer += Time.delta_unscaled;

        if (_timer > _time)
        {
            _timer = 0.f;
            widget->setState(EState::STATE_NONE);
            widget->propagateState(EState::STATE_IN);
            return;
        }

        setTime(widget);
    }

    void CEffect_FadeIn::renderInMenu()
    {
        ImGui::DragFloat("Timer", &_timer, kDebugMenuSensibility);
        ImGui::DragFloat("Fade Time", &_time, kDebugMenuSensibility);
        ImGui::DragFloat("Factor", &_factor, kDebugMenuSensibility);
    }

    // Fade out

    void CEffect_FadeOut::setTime(CWidget* widget)
    {
        setTimeFactor(widget, clampf(1.f - (_timer / _time), 0.f, 1.f));
    }

    void CEffect_FadeOut::update(CWidget* widget, float elapsed)
    {
        if (widget->getState() != EState::STATE_OUT)
            return;

        _timer += Time.delta_unscaled;

        if (_timer > _time)
        {
            _timer = 0.f;
            widget->setState(EState::STATE_CLEAR);
            widget->propagateState(EState::STATE_OUT);
            return;
        }

        setTime(widget);
    }
}
