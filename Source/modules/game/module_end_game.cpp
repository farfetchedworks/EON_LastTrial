#include "mcv_platform.h"
#include "module_end_game.h"
#include "engine.h"
#include "modules/module_manager.h"
#include "modules/module_entities.h"
#include "render/render_module.h"
#include "modules/module_boot.h"
#include "input/input_module.h"
#include "ui/ui_module.h"
#include "ui/ui_widget.h"
#include "ui/widgets/ui_video.h"
#include "audio/module_audio.h"

bool ModuleEONEndGame::start()
{
    debugging = true;
    CApplication::get().changeMouseState(debugging, false);

    input = CEngine::get().getInput(input::MENU);
    assert(input);

    EngineUI.activateWidget("eon_credits");

    ui::CVideo* video = (ui::CVideo*)EngineUI.getWidget("eon_credits");
    bool is_ok = video->getVideoParams()->video->reset();
    assert(is_ok);

    _menuController.setInput(input);
    _menuController.bindButton("continue_btn_end", std::bind(&ModuleEONEndGame::onContinue, this));
    _menuController.bindButton("exit_btn_end", std::bind(&ModuleEONEndGame::onExit, this));

    _menuController.reset();
    _menuController.selectOption(0);
    
    return is_ok;
}

void ModuleEONEndGame::stop()
{
    EngineUI.deactivateWidget("modal_black", false);
    EngineUI.deactivateWidget("eon_credits", false);
    EngineUI.deactivateWidget("eon_end_game");
}

void ModuleEONEndGame::update(float dt)
{
    ui::CVideo* video = (ui::CVideo*)EngineUI.getWidget("eon_credits");
    if (video->getVideoParams()->video->hasFinished())
    {
        EngineUI.activateWidget("eon_end_game");
        _menuController.update(dt);
    }
}

void ModuleEONEndGame::onContinue()
{
    auto& mm = CEngine::get().getModuleManager();

    mm.resetModules();
    mm.changeToGamestate("main_menu");

    // FMOD end ending theme
    EngineAudio.stopCurMusicEvent();
}

void ModuleEONEndGame::onExit()
{
    CApplication::get().exit();
}
