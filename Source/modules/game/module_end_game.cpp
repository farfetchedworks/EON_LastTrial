#include "mcv_platform.h"
#include "module_end_game.h"
#include "engine.h"
#include "modules/module_manager.h"
#include "modules/module_entities.h"
#include "render/render_module.h"
#include "input/input_module.h"
#include "ui/ui_module.h"
#include "ui/ui_widget.h"
#include "fmod_studio.hpp"

bool ModuleEONEndGame::start()
{
    EngineUI.activateWidget("eon_end_game");
    EngineAudio.stop();
    Entities.clearDebugList();
    return true;
}

void ModuleEONEndGame::stop()
{

}

void ModuleEONEndGame::update(float dt)
{

}
