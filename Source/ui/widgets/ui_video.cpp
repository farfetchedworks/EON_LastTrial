#include "mcv_platform.h"
#include "ui/widgets/ui_video.h"
#include "ui/ui_module.h"
#include "engine.h"

namespace ui
{
    void CVideo::update(float dt)
    {
        if(videoParams.video)
            videoParams.video->update(dt);
    }

    void CVideo::render()
    {
        CEngine::get().getUI().renderVideo(_worldTransform, videoParams, _worldSize);
    }

    void CVideo::renderInMenu()
    {
        CWidget::renderInMenu();
        videoParams.renderInMenu();
    }
}