#pragma once

#include "ui/ui_widget.h"
#include "ui/ui_params.h"

namespace ui
{
    class CProgressBar : public CWidget
    {
    public:
        std::string_view getType() const override { return "Progress Bar"; }

        void render() override;
        void renderInMenu() override;
        TImageParams* getImageParams() override { return &imageParams; }

       TImageParams imageParams;
       float ratio = 1.f;
    };
}
