#pragma once

#include "ui/ui_widget.h"
#include "ui/ui_params.h"

namespace ui
{
    class CImage : public CWidget
    {
    public:
        std::string_view getType() const override { return "Image"; }

        void render() override;
        void renderInMenu() override;
        TImageParams* getImageParams() override { return &imageParams; }

       TImageParams imageParams;
    };
}
