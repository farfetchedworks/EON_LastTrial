#pragma once

#include "ui/ui_widget.h"
#include "ui/ui_params.h"

namespace ui
{
    class CText : public CWidget
    {
    public:
        std::string_view getType() const override { return "Text"; }

        void render() override;
        void renderInMenu() override;

       TTextParams textParams;
    };
}
