#include "mcv_platform.h"
#include "ui_checkbox.h"
#include "ui/ui_module.h"
#include "engine.h"

namespace ui
{
    CCheckbox::CCheckbox()
    {
        TStateParams defaultState;
        defaultState.imageParams.texture = Resources.get("data/textures/ui/subvert/settings/disabled.dds")->as<CTexture>();
        states["default"] = defaultState;

        TStateParams selectedState;
        selectedState.imageParams.texture = Resources.get("data/textures/ui/subvert/settings/hover_disabled.dds")->as<CTexture>();
        states["selected"] = selectedState;

        TStateParams enabledState;
        enabledState.imageParams.texture = Resources.get("data/textures/ui/subvert/settings/enabled.dds")->as<CTexture>();
        states["enabled"] = enabledState;
    }

    bool CCheckbox::toggle()
    {
        _enabled = !_enabled;

        std::string path = "data/textures/ui/subvert/settings/";

        states["default"].imageParams.texture = Resources.get( 
            path + (_enabled ? "enabled.dds" : "disabled.dds"))->as<CTexture>();
        states["selected"].imageParams.texture = Resources.get(
            path + (_enabled ? "hover_enabled.dds" : "hover_disabled.dds"))->as<CTexture>();
        return _enabled;
    }
}