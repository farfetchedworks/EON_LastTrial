#pragma once
#include "ui/ui_widget.h"
#include "ui/widgets/ui_button.h"
#include "ui/ui_params.h"

namespace ui
{
    class CCheckbox : public CButton
    {
    public:

        CCheckbox();
        std::string_view getType() const override { return "Checkbox"; }
        bool toggle();

    private:        
        bool _enabled = false;
    };
}
