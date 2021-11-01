#include "mcv_platform.h"
#include "ui/widgets/ui_bar.h"
#include "ui/ui_module.h"
#include "engine.h"

namespace ui
{
    void CProgressBar::render()
    {
        const VEC2 progressSize(_worldSize.x * ratio, _worldSize.y);

        TImageParams barParams = imageParams;
        barParams.maxUV.x *= ratio;

        CEngine::get().getUI().renderBitmap(_worldTransform, barParams, progressSize);
    }

    void CProgressBar::renderInMenu()
    {
        CWidget::renderInMenu();

        imageParams.renderInMenu();

        ImGui::DragFloat("Ratio", &ratio, kDebugMenuSensibility, 0.f, 1.f);
    }
}