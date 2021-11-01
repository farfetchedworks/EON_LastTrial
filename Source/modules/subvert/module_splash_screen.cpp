#include "mcv_platform.h"
#include "modules/subvert/module_splash_screen.h"
#include "engine.h"
#include "modules/module_manager.h"
#include "render/render_module.h"
#include "ui/ui_module.h"
#include "ui/ui_widget.h"

bool ModuleSubvertSplashScreen::start()
{
    _timer = 3.f;
    EngineRender.setClearColor({0.f, 0.f, 0.f, 1.f});
    EngineUI.activateWidget("subvert_splash_screen");
    return true;
}

void ModuleSubvertSplashScreen::stop()
{
    EngineUI.deactivateWidget("subvert_splash_screen");
}

void ModuleSubvertSplashScreen::update(float dt)
{
    _timer -= dt;
    
    if (_timer <= 0.f)
    {
        CEngine::get().getModuleManager().changeToGamestate("main_menu");
    }
}
