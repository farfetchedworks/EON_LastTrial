#include "mcv_platform.h"
#include "ui_effect_anim_uvs.h"
#include "ui/widgets/ui_image.h"

namespace ui
{
    void CEffect_AnimateUV::update(CWidget* image, float elapsed)
    {
        TImageParams* params = image->getImageParams();
        if (!params)
        {
            return;
        }

        params->minUV += _speed * elapsed;
        params->maxUV += _speed * elapsed;
    }

    void CEffect_AnimateUV::renderInMenu()
    {
        ImGui::DragFloat2("Speed", &_speed.x, kDebugMenuSensibility);
    }
}
