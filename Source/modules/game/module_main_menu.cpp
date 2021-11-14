#include "mcv_platform.h"
#include "module_main_menu.h"
#include "engine.h"
#include "modules/module_manager.h"
#include "render/render_module.h"
#include "input/input_module.h"
#include "ui/ui_module.h"
#include "ui/ui_widget.h"
#include "fmod_studio.hpp"

bool ModuleEONMainMenu::start()
{
    debugging = true;
    CApplication::get().changeMouseState(debugging, false);

    input = CEngine::get().getInput(input::MENU);
    assert(input);

    EngineUI.activateWidget("eon_main_menu");

    _menuController.setInput(input);
    _menuController.bind("start_btn", std::bind(&ModuleEONMainMenu::onNewGame, this));
    _menuController.bind("settings_btn", std::bind(&ModuleEONMainMenu::onSettings, this));
    _menuController.bind("exit_btn", std::bind(&ModuleEONMainMenu::onExit, this));

    _menuController.reset();
    _menuController.selectOption(0);

    // Start title theme
    EngineAudio.loadBank("OutOfGame.bank");
    EngineAudio.postMusicEvent("Music/Title_Theme");

    return true;
}

void ModuleEONMainMenu::stop()
{
    EngineUI.deactivateWidget("eon_main_menu");
    EngineAudio.stopCurMusicEvent();
    EngineAudio.unloadBank("OutOfGame.bank");
}

void ModuleEONMainMenu::update(float dt)
{
    _menuController.update(dt);

    // Exit game
    if (input->getButton("exit_game").getsPressed()) {
        onExit();
    }
}

void ModuleEONMainMenu::onNewGame()
{
    CModuleManager& modules = CEngine::get().getModuleManager();
    modules.changeToGamestate("loading");
}

void ModuleEONMainMenu::onExit()
{
    CApplication::get().exit();
}
