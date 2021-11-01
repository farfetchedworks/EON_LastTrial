#include "mcv_platform.h"
#include "ui/widgets/ui_image.h"
#include "ui/ui_module.h"
#include "engine.h"

namespace ui
{
    void CImage::render()
    {
        if (!imageParams.texture)
            return;

        CEngine::get().getUI().renderBitmap(_worldTransform, imageParams, _worldSize);
    }

    void CImage::renderInMenu()
    {
        CWidget::renderInMenu();
        imageParams.renderInMenu();
    }
}