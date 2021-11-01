#include "mcv_platform.h"
#include "modules/subvert/module_you_died.h"
#include "engine.h"
#include "modules/module_manager.h"
#include "render/render_module.h"
#include "input/input_module.h"
#include "ui/ui_module.h"
#include "ui/ui_widget.h"
#include "fmod_studio.hpp"

bool ModuleSubvertYouDied::start()
{
    debugging = true;
    CApplication::get().changeMouseState(true, false);

    EngineRender.setClearColor({ 0.f, 0.f, 0.f, 1.f });
    _player1 = CEngine::get().getInput(input::PLAYER_1);
    assert(_player1);

    EngineUI.activateWidget("subvert_you_died");
    EngineUI.deactivateWidget("subvert_hud");

    _menuController.bind("bt_continue", std::bind(&ModuleSubvertYouDied::onContinue, this));
    _menuController.bind("bt_surrender", std::bind(&ModuleSubvertYouDied::onExit, this));

    _menuController.reset();
    _menuController.selectOption(0);

    // Start title theme
    // EngineAudio.loadBank("OutOfGame.bank");
    // EngineAudio.postMusicEvent("Music/Title_Theme");

    return true;
}
void ModuleSubvertYouDied::stop()
{
    EngineUI.deactivateWidget("subvert_you_died");
    
    /*fmod_event->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
    fmod_event->release();
    EngineAudio.unloadBank("OutOfGame.bank");*/
}

void ModuleSubvertYouDied::update(float dt)
{
    _menuController.update(dt);

    // Exit game
    if (PlayerInput["exit_game"].getsPressed()) {
        onExit();
    }
}

void ModuleSubvertYouDied::onContinue()
{
    CEngine::get().getModuleManager().changeToGamestate("playing");
}

void ModuleSubvertYouDied::onSurrender()
{
    CEngine::get().getModuleManager().changeToGamestate("main_menu");
}

void ModuleSubvertYouDied::onExit()
{
    CApplication::get().exit();
}
