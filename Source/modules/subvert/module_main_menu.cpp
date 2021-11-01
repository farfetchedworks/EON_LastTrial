#include "mcv_platform.h"
#include "modules/subvert/module_main_menu.h"
#include "engine.h"
#include "modules/module_manager.h"
#include "render/render_module.h"
#include "input/input_module.h"
#include "ui/ui_module.h"
#include "ui/ui_widget.h"
#include "fmod_studio.hpp"

bool ModuleSubvertMainMenu::start()
{
    _player1 = CEngine::get().getInput(input::PLAYER_1);
    assert(_player1);

    EngineUI.activateWidget("subvert_main_menu");

    _menuController.bind("bt_new_game", std::bind(&ModuleSubvertMainMenu::onNewGame, this));
    _menuController.bind("bt_exit", std::bind(&ModuleSubvertMainMenu::onExit, this));

    _menuController.reset();
    _menuController.selectOption(0);

    // Start title theme
    EngineAudio.loadBank("OutOfGame.bank");
    EngineAudio.postMusicEvent("Music/Title_Theme");

    return true;
}

void ModuleSubvertMainMenu::stop()
{
    EngineUI.deactivateWidget("subvert_main_menu");
    EngineUI.getWidget("text_loading")->setVisible(false);
    
    EngineAudio.stopCurMusicEvent();
    EngineAudio.unloadBank("OutOfGame.bank");
}

void ModuleSubvertMainMenu::update(float dt)
{
    _menuController.update(dt);

    // Exit game
    if (PlayerInput["exit_game"].getsPressed()) {
        onExit();
    }
}

void ModuleSubvertMainMenu::onNewGame()
{
    CModuleManager& modules = CEngine::get().getModuleManager();
    EngineUI.getWidget("text_loading")->setVisible(true);
    modules.changeToGamestate("playing");
}

void ModuleSubvertMainMenu::onExit()
{
    CApplication::get().exit();
}
