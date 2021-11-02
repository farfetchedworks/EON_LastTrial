#include "mcv_platform.h"
#include "module_splash_screen.h"
#include "engine.h"
#include "modules/module_manager.h"
#include "render/render_module.h"
#include "ui/ui_module.h"
#include "ui/ui_widget.h"

bool ModuleEONSplashScreen::start()
{
    _timer = 3.f;
    EngineRender.setClearColor({0.f, 0.f, 0.f, 1.f});
    EngineUI.activateWidget("eon_splash_screen");
    return true;
}

void ModuleEONSplashScreen::stop()
{
    EngineUI.deactivateWidget("eon_splash_screen");
}

void ModuleEONSplashScreen::update(float dt)
{
    _timer -= dt;
    
    if (_timer <= 0.f)
    {
        CEngine::get().getModuleManager().changeToGamestate("main_menu");
    }
}
