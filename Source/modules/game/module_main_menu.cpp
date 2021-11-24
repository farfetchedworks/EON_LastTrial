#include "mcv_platform.h"
#include "module_main_menu.h"
#include "engine.h"
#include "modules/module_manager.h"
#include "render/render_module.h"
#include "input/input_module.h"
#include "ui/ui_module.h"
#include "ui/ui_widget.h"
#include "ui/widgets/ui_text.h"
#include "ui/ui_params.h"
#include "fmod_studio.hpp"
#include "audio/module_audio.h"

bool ModuleEONMainMenu::start()
{
    debugging = true;
    CApplication::get().changeMouseState(debugging, false);

    input = CEngine::get().getInput(input::MENU);
    assert(input);

    EngineUI.deactivateWidget("eon_hud");

    // Set version
    ui::CWidget* wMenu = EngineUI.getWidget("eon_main_menu");
    assert(wMenu);
    ui::CText* wVersion = (ui::CText*)wMenu->getChildByName("version");
    wVersion->textParams.text = Engine.getVersion();
    EngineUI.activateWidget(wMenu);

    _menuController.setInput(input);
    _menuController.bind("start_btn", std::bind(&ModuleEONMainMenu::onNewGame, this));
    _menuController.bind("settings_btn", std::bind(&ModuleEONMainMenu::onSettings, this));
    _menuController.bind("exit_btn_menu", std::bind(&ModuleEONMainMenu::onExit, this));

    _menuController.reset();
    _menuController.selectOption(0);

    // Start title theme
    EngineAudio.postMusicEvent("Music/Title_Theme");

    return true;
}

void ModuleEONMainMenu::stop()
{
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

void ModuleEONMainMenu::onExit()
{
    CApplication::get().exit();
}
