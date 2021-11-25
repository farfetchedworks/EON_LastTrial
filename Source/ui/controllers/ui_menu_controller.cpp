#include "mcv_platform.h"
#include "ui/controllers/ui_menu_controller.h"
#include "ui/ui_module.h"
#include "ui/widgets/ui_button.h"
#include "input/input_module.h"
#include "engine.h"
#include "audio/module_audio.h"

namespace ui
{
    void CMenuController::reset()
    {
        for (TOption& option : _options)
        {
            option.button->changeToState("default");
        }
    }

    void CMenuController::update(float elapsed)
    {
        assert(input);
        if (!input)
            return;

        processMouseHover();

        if (input->getButton("menu_down").getsPressed())
        {
            nextOption();
        }
        else if (input->getButton("menu_up").getsPressed())
        {
            prevOption();
        }
        else if ((isHoverButton && input->getButton("mouse_left").getsPressed()) || input->getButton("menu_confirm").getsPressed())
        {
            highlightOption();
        }
        else if ((isHoverButton && input->getButton("mouse_left").getsReleased()) || input->getButton("menu_confirm").getsReleased())
        {
            confirmOption();
        }
    }

    void CMenuController::bind(const std::string& buttonName, Callback callback)
    {
        CButton* button = dynamic_cast<CButton*>(CEngine::get().getUI().getWidget(buttonName));
        assert(button);
        if(!button) return;

        TOption option;
        option.button = button;
        option.callback = callback;
        _options.push_back(option);
    }

    void CMenuController::nextOption()
    {
        selectOption(std::clamp<int>(_currentOption + 1, 0, static_cast<int>(_options.size()) - 1));

        // FMOD hover event
        EngineAudio.postEvent("UI/Hover");
    }

    void CMenuController::prevOption()
    {
        selectOption(std::clamp<int>(_currentOption - 1, 0, static_cast<int>(_options.size()) - 1));

        // FMOD hover event
        EngineAudio.postEvent("UI/Hover");
    }

    void CMenuController::selectOption(int idx, bool hover_audio)
    {
        if (idx == _currentOption)
            return;

        if (idx < 0 || idx >= _options.size())
        {
            if (defaultIfUndefined)
            {
                for (auto& option : _options)
                {
                    _currentOption = kUndefinedOption;
                    option.button->changeToState("default");
                }
            }
            return;
        }

        if (_currentOption != kUndefinedOption)
        {
            _options.at(_currentOption).button->changeToState("default");
        }

        _options.at(idx).button->changeToState("selected");
        _currentOption = idx;

        // FMOD hover event
        if (hover_audio)
            EngineAudio.postEvent("UI/Hover");
    }

    void CMenuController::highlightOption()
    {
        if (_currentOption < 0 || _currentOption >= _options.size())
            return;

        _options.at(_currentOption).button->changeToState("pressed");

        // Fmod event
        EngineAudio.postEvent("UI/Highlight");
    }

    void CMenuController::confirmOption()
    {
        if (_currentOption < 0 || _currentOption >= _options.size())
            return;

        TOption& option = _options.at(_currentOption);
        option.button->changeToState("selected");
        option.callback();

        // FMOD events
        if (!option.button->getName().compare("start_btn"))
            EngineAudio.postEvent("UI/Start_Game");
        else
            EngineAudio.postEvent("UI/Enter");
    }

    void CMenuController::processMouseHover()
    {
        int hoveredButton = -1;
        VEC2 mouse_pos = input->getMousePosition()  * EngineUI.getResolution();
        bool mouse_active = mouse_pos != last_mouse_pos;

        if (mouse_active)
        {
            last_mouse_pos = mouse_pos;
            hoveredButton = getButton(mouse_pos);
            selectOption(hoveredButton, true);
            isHoverButton = hoveredButton != -1;
        }

    }

    int CMenuController::getButton(VEC2 mouse_pos)
    {
        for (int i = 0; i < _options.size(); i++)
        {
            auto button = _options[i].button;
            VEC3 worldScale = button->getWorldTransform().Scale();
            VEC2 worldSize = button->getSize() * VEC2(worldScale);
            VEC2 worldPos = VEC2(button->getWorldTransform().Translation());

            if (isPointInRectangle(mouse_pos, worldSize, worldPos)) {
                return i;
            }
        }

        return -1;
    }
}
