#include "mcv_platform.h"
#include "ui/widgets/ui_button.h"
#include "ui/ui_module.h"
#include "engine.h"

namespace ui
{
    void CButton::render()
    {
        CModule& ui = CEngine::get().getUI();

        TImageParams& iParams = _currentState ? _currentState->imageParams : imageParams;
        TTextParams& tParams = _currentState ? _currentState->textParams : textParams;
            
        ui.renderBitmap(_worldTransform, iParams, _worldSize);
        ui.renderText(_worldTransform, tParams, _worldSize);
    }

    void CButton::renderInMenu()
    {
        CWidget::renderInMenu();

        imageParams.renderInMenu();
        textParams.renderInMenu();
    }

    void CButton::addState(const std::string& name, TStateParams& stateParams)
    {
        states[name] = stateParams;
    }

    void CButton::changeToState(const std::string& name)
    {
        auto it = states.find(name);
        if(it == states.cend()) return;

        _currentState = &it->second;
    }

    TImageParams* CButton::getImageParams()
    {
        return _currentState ? &_currentState->imageParams : &imageParams;
    }
}