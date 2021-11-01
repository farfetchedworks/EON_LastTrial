#pragma once

#include "ui/ui_widget.h"
#include "ui/ui_params.h"

namespace ui
{
    class CButton : public CWidget
    {
    public:
        struct TStateParams
        {
            TImageParams imageParams;
            TTextParams textParams;
        };

        std::string_view getType() const override { return "Button"; }

        void render() override;
        void renderInMenu() override;
        TImageParams* getImageParams() override;

        void addState(const std::string& name, TStateParams& stateParams);
        void changeToState(const std::string& name);

        TImageParams imageParams;
        TTextParams textParams;

        std::map<std::string, TStateParams> states;

    private:        
        TStateParams* _currentState = nullptr;
    };
}
