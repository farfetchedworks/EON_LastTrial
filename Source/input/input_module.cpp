#include "mcv_platform.h"
#include "engine.h"
#include "input/input_module.h"
#include "input/input_device.h"
#include "input/interfaces/interface_vibration.h"

namespace input
{
    const TButton TButton::dummy;
    std::map<std::string, TButtonDef> CModule::_definitions;

    CModule::CModule(const std::string& name, int id)
        : IModule(name)
        , _id(id)
        , _mapping(*this)
    {
        if (id != MENU)
        {
            _playerInput = true;
        }
    }

    bool CModule::start()
    {
        if (_playerInput)
        {
            blockInput();
        }

        return true;
    }

    void CModule::update(float delta)
    {
        // Block input in the gameplay state
        // bool inGame = CEngine::get().getModuleManager().inGamestate("playing");

        if (_blocked)
            return;

        delta = Time.delta_unscaled;

        for (auto device : _devices)
        {
          PROFILE_FUNCTION("FetchData");
          device->fetchData(_keyboard);
          device->fetchData(_mouse);
          device->fetchData(_pad);
        }

        _keyboard.update(delta);
        _mouse.update(delta);
        _pad.update(delta);
        _mapping.update(delta);
    }

    void CModule::registerDevice(IDevice* device)
    {
        // TODO: check if device is already in the list
        _devices.push_back(device);
    }

    void CModule::loadMapping(const std::string& filename)
    {
        _mapping.load(filename);
    }

    void CModule::clearInput()
    {
        for (auto device : _devices)
        {
            device->clearData();
        }
    }

    const TButton& CModule::getButton(const std::string& name) const
    {
        const TButtonDef* buttonDef = getButtonDefinition(name);
        if (buttonDef)
        {
            return getButton(*buttonDef);
        }

        return _mapping.getButton(name);
    }

    const TButton& CModule::getKey(EKeyboardKey key) const
    {
        return _keyboard.keys[key];
    }

    const TButton& CModule::getMouseButton(EMouseButton bt) const
    {
        return _mouse.buttons[static_cast<int>(bt)];
    }

    const TButton& CModule::getPadButton(EPadButton bt) const
    {
        return _pad.buttons[static_cast<int>(bt)];
    }

    void CModule::feedback(const TVibrationData& data)
    {
        for (auto device : _devices)
        {
            device->feedback(data);
        }
    }

    const TButton& CModule::getButton(const TButtonDef& def) const
    {
        switch (def.type)
        {
            case EInterface::KEYBOARD:  return _keyboard.keys[def.id];
            case EInterface::MOUSE:     return _mouse.buttons[def.id];
            case EInterface::PAD:       return _pad.buttons[def.id];
            default:                    return TButton::dummy;
        }
    }

    const TButton& CModule::operator[](const std::string& name) const
    {
        return getButton(name);
    }

    const TButton& CModule::operator[](EKeyboardKey key) const
    {
        return getKey(key);
    }

    const TButton& CModule::operator[](EPadButton bt) const
    {
        return getPadButton(bt);
    }

    void CModule::loadDefinitions(const std::string& filename)
    {
        const auto parseDefs = [&](const json& jData, EInterface type)
        {
            int idx = 0;
            for (auto jButton : jData)
            {
                _definitions.insert({ jButton, {type, idx++} });
            }
        };

        auto jData = loadJson(filename);
        parseDefs(jData["keyboard"], EInterface::KEYBOARD);
        parseDefs(jData["mouse"], EInterface::MOUSE);
        parseDefs(jData["pad"], EInterface::PAD);
    }

    const TButtonDef* CModule::getButtonDefinition(const std::string& name)
    {
        auto it = _definitions.find(name);
        return it != _definitions.cend() ? &it->second : nullptr;
    }

    const std::string& CModule::getButtonName(const TButtonDef& def)
    {
        auto it = std::find_if(_definitions.cbegin(), _definitions.cend(), [def](const auto& entry) { return entry.second == def; });
        static std::string kEmpty;
        return it != _definitions.cend() ? it->first : kEmpty;
    }

    void CModule::renderInMenu()
    {
        const auto printButton = [](const TButton& button, const std::string& label, bool centerValue = false)
        {
            const float value = centerValue ? 0.5f + button.value * 0.5f : button.value;
            ImGui::ProgressBar(value, ImVec2(-1, 0), label.c_str());
        };

        if (ImGui::TreeNode(getName().c_str()))
        {
            ImGui::Checkbox("Blocked", &_blocked);

            if (ImGui::TreeNode("Keyboard"))
            {
                for (int idx = 0; idx < NUM_KEYBOARD_KEYS; ++idx)
                {
                    printButton(_keyboard.keys[idx], getButtonName(TButtonDef{ EInterface::KEYBOARD, idx }));
                }
                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Mouse"))
            {
                ImGui::DragFloat2("Position", &_mouse.position.x);
                ImGui::DragFloat2("Delta", &_mouse.delta.x);
                ImGui::ProgressBar(fabsf((float)_mouse.wheelSteps) / 3.f, ImVec2(-1, 0), "Wheel");

                for (int idx = 0; idx < NUM_MOUSE_BUTTONS; ++idx)
                {
                    printButton(_mouse.buttons[idx], getButtonName(TButtonDef{ EInterface::MOUSE, idx }));
                }
                ImGui::Separator();
                ImGui::DragFloat("Delta X", &_mouse.delta.x);
                ImGui::DragFloat("Delta Y", &_mouse.delta.y);
                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Pad"))
            {
                ImGui::Text("Connected: %s", _pad.connected ? "YES" : "no");
                for (int idx = 0; idx < NUM_PAD_BUTTONS; ++idx)
                {
                    printButton(_pad.buttons[idx], getButtonName(TButtonDef{ EInterface::PAD, idx }), isAnalog(EPadButton(idx)));
                }
                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Mapping"))
            {
                const std::map<std::string, TMappedButton>& mappedButtons = _mapping.getMappedButtons();

                for (auto& entry : mappedButtons)
                {
                    printButton(entry.second.button, entry.first, true);
                }
                ImGui::TreePop();
            }

            ImGui::TreePop();
        }
    }
}
