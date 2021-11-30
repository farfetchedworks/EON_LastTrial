#include "mcv_platform.h"
#include "module_settings.h"
#include "engine.h"
#include "input/input_module.h"
#include "ui/ui_module.h"
#include "ui/ui_widget.h"
#include "ui/widgets/ui_text.h"
#include "ui/ui_params.h"
#include "fmod_studio.hpp"
#include "audio/module_audio.h"
#include "components/postfx/comp_render_reflections.h"
#include "components/postfx/comp_render_ao.h"
#include "components/postfx/comp_godrays.h"

bool CModuleSettings::start()
{
    debugging = true;
    CApplication::get().changeMouseState(debugging, false);

    if (!input)
    {
        initSettings();
    }

    _menuController.reset();
    _menuController.selectOption(0);

    if (_callingScreen == "playing")
    {
        EngineUI.activateWidget("modal_black_alpha");
        EngineUI.deactivateWidget("eon_pause");
    }

    EngineUI.activateWidget("eon_settings");

    return true; 
}

void CModuleSettings::initSettings()
{
    input = CEngine::get().getInput(input::MENU);
    assert(input);

    _menuController.setInput(input);

    loadDefaultSettings();

    // Bind checkboxes
    for (auto& s : _settings)
    {
        _menuController.bindCheckbox(s.name, std::bind(&CModuleSettings::onChangeSetting, this, std::placeholders::_1, std::placeholders::_2));
    }

    _menuController.bindButton("back_btn", std::bind(&CModuleSettings::onGoBack, this));
}

void CModuleSettings::loadDefaultSettings()
{
    // Callbacks could be used to change settings "in game"

    TSetting vSyncSetting;
    vSyncSetting.enabled = true;
    vSyncSetting.name = "vsync_cb";
    vSyncSetting.callback = [](bool enabled) {
        Render.setVSyncEnabled(enabled);
    };

    TSetting VolumetricSetting;
    VolumetricSetting.enabled = true;
    VolumetricSetting.name = "godrays_cb";
    VolumetricSetting.callback = [](bool enabled) {

        CEntity* camera_mixed = getEntityByName("camera_mixed");
        if (camera_mixed)
        {
            TCompGodRays* vl = camera_mixed->get<TCompGodRays>();
            assert(vl);
            vl->setEnabled(enabled);
        }
    };

    TSetting SSRSetting;
    SSRSetting.enabled = true;
    SSRSetting.name = "ssr_cb";
    SSRSetting.callback = [](bool enabled) {

        CEntity* camera_mixed = getEntityByName("camera_mixed");
        if (camera_mixed)
        {
            TCompSSReflections* ssr = camera_mixed->get<TCompSSReflections>();
            assert(ssr);
            ssr->setEnabled(enabled);
        }
    };

    TSetting SSAOSetting;
    SSAOSetting.enabled = true;
    SSAOSetting.name = "ssao_cb";
    SSAOSetting.callback = [](bool enabled) {

        CEntity* camera_mixed = getEntityByName("camera_mixed");
        if (camera_mixed)
        {
            TCompRenderAO* ao = camera_mixed->get<TCompRenderAO>();
            assert(ao);
            ao->setEnabled(enabled);
        }
    };

    _settings.push_back(vSyncSetting);
    _settings.push_back(VolumetricSetting);
    _settings.push_back(SSRSetting);
    _settings.push_back(SSAOSetting);
}

TSetting* CModuleSettings::getSetting(const std::string& settingName)
{
    for (auto& s : _settings)
    {
        if (s.name == settingName)
            return &s;
    }

    return nullptr;
}

void CModuleSettings::stop()
{
    // Playing state will deactivate the widget after 
    // deactivating pause one
    if (_callingScreen != "playing")
    {
        EngineUI.deactivateWidget("eon_settings");
    }
}

void CModuleSettings::update(float dt)
{
    _menuController.update(dt);

    if (input->getButton("pause_game").getsPressed()) {
        onGoBack();
    }
}

void CModuleSettings::onChangeSetting(const std::string& buttonName, bool enabled)
{
    TSetting* s = getSetting(buttonName);
    assert(s && "Incorrect button name for checkbox");
    s->enabled = enabled;
    if(s->callback)
        s->callback(enabled);
}

void CModuleSettings::onGoBack()
{
    CModuleManager& modules = CEngine::get().getModuleManager();

    if (_callingScreen == "playing")
    {
        modules.changeToGamestate("playing");
        _toGameplay = true;
    }
    else
    {
        modules.changeToGamestate("main_menu");
    }
}