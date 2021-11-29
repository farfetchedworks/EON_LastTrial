#include "mcv_platform.h"
#include "module_main_menu.h"
#include "engine.h"
#include "modules/module_manager.h"
#include "render/render_module.h"
#include "modules/module_settings.h"
#include "input/input_module.h"
#include "ui/ui_module.h"
#include "ui/ui_widget.h"
#include "ui/widgets/ui_text.h"
#include "ui/ui_params.h"
#include "audio/module_audio.h"

bool ModuleEONMainMenu::start()
{
    debugging = true;
    CApplication::get().changeMouseState(debugging, false);

    EngineUI.deactivateWidget("eon_hud");

    // Start title theme
    EngineAudio.postMusicEvent("Music/Title_Theme");

    ui::CWidget* wMenu = EngineUI.getWidget("eon_main_menu");
    assert(wMenu);

    // if there's input, we have been here before..
    if (!input)
    {
        // Init settings the first time we enter
        Settings.initSettings();

        input = CEngine::get().getInput(input::MENU);
        assert(input);

        // Set version
        ui::CText* wVersion = (ui::CText*)wMenu->getChildByName("version");
        wVersion->textParams.text = Engine.getVersion();

        _menuController.setInput(input);
        _menuController.bindButton("start_btn", std::bind(&ModuleEONMainMenu::onNewGame, this));
        _menuController.bindButton("settings_btn", std::bind(&ModuleEONMainMenu::onSettings, this));
        _menuController.bindButton("credits_btn", std::bind(&ModuleEONMainMenu::onCredits, this));
        _menuController.bindButton("exit_btn_menu", std::bind(&ModuleEONMainMenu::onExit, this));

        _menuController.reset();
        _menuController.selectOption(0);
    }

    EngineUI.activateWidget(wMenu);

    // We come from the settings
    if (_toSettings)
    {
        auto children = wMenu->getChildren();
        for (auto child : children)
        {
            child->setVisible(true);
        }

        _toSettings = false;
    }

    return true;
}

void ModuleEONMainMenu::stop()
{
    if (_toSettings)
        return;

    EngineUI.deactivateWidget("eon_main_menu");
}

void ModuleEONMainMenu::update(float dt)
{
    _menuController.update(dt);
}

void ModuleEONMainMenu::onNewGame()
{
    CModuleManager& modules = CEngine::get().getModuleManager();
    modules.changeToGamestate("intro");
}

void ModuleEONMainMenu::onSettings()
{
    _toSettings = true;

    auto children = EngineUI.getWidget("eon_main_menu")->getChildren();
    for (auto child : children)
    {
        child->setVisible(child->getChildren().size() == 0);
    }

    Settings.setCaller("main_menu");
    CModuleManager& modules = CEngine::get().getModuleManager();
    modules.changeToGamestate("settings");
}

void ModuleEONMainMenu::onCredits()
{

}

void ModuleEONMainMenu::onExit()
{
    CApplication::get().exit();
}
