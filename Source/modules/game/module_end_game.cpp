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
#include "fmod_studio.hpp"

bool ModuleEONEndGame::start()
{
    debugging = true;
    CApplication::get().changeMouseState(debugging, false);

    input = CEngine::get().getInput(input::MENU);
    assert(input);

    EngineUI.activateWidget("eon_end_game");

    _menuController.setInput(input);
    _menuController.bindButton("continue_btn_end", std::bind(&ModuleEONEndGame::onContinue, this));
    _menuController.bindButton("exit_btn_end", std::bind(&ModuleEONEndGame::onExit, this));

    _menuController.reset();
    _menuController.selectOption(0);

    // audio??
    // ...
    
    return true;
}

void ModuleEONEndGame::stop()
{
    EngineUI.deactivateWidget("modal_black", false);
    EngineUI.deactivateWidget("eon_end_game");
}

void ModuleEONEndGame::update(float dt)
{
    _menuController.update(dt);
}

void ModuleEONEndGame::onContinue()
{
    auto& mm = CEngine::get().getModuleManager();

    mm.resetModules();
    mm.changeToGamestate("main_menu");
}

void ModuleEONEndGame::onExit()
{
    CApplication::get().exit();
}
