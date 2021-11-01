#include "mcv_platform.h"
#include "ui/widgets/ui_text.h"
#include "ui/ui_module.h"
#include "engine.h"

namespace ui
{
    void CText::render()
    {
        CEngine::get().getUI().renderText(_worldTransform, textParams, _worldSize);
    }

    void CText::renderInMenu()
    {
        CWidget::renderInMenu();

        textParams.renderInMenu();
    }
}