#pragma once

#include "ui/ui_widget.h"
#include "ui/ui_params.h"

namespace ui
{
    class CVideo : public CWidget
    {
    public:
        std::string_view getType() const override { return "Video"; }

        void update(float dt) override;
        void render() override;
        void renderInMenu() override;
        TVideoParams* getVideoParams() override { return &videoParams; }

       TVideoParams videoParams;
    };
}
