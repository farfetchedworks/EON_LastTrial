#include "mcv_platform.h"
#include "module_splash_screen.h"
#include "engine.h"
#include "modules/module_manager.h"
#include "render/render_module.h"
#include "ui/ui_module.h"
#include "ui/ui_widget.h"
#include "input/input_module.h"
#include "ui/widgets/ui_video.h"
#include "audio/module_audio.h"

bool ModuleEONSplashScreen::start()
{
    debugging = false;
    CApplication::get().changeMouseState(debugging, false);

    input = CEngine::get().getInput(input::MENU);
    assert(input);

    EngineRender.setClearColor({0.f, 0.f, 0.f, 1.f});
    EngineUI.activateWidget("eon_splash_screen");

    // Start title theme
    EngineAudio.postMusicEvent("Music/Title_Theme");
    fmod_event = EngineAudio.post2DEventGetInst("UI/Splash_Screen");

    return true;
}

void ModuleEONSplashScreen::stop()
{
    EngineUI.deactivateWidget("eon_splash_screen");
    EngineAudio.stopEvent(fmod_event);
}

void ModuleEONSplashScreen::update(float dt)
{
    if (input->getButton("pause_game").getsPressed()) {
        CEngine::get().getModuleManager().changeToGamestate("main_menu");
        EngineAudio.setMusicTimelinePosition(23000);
    }
        
    ui::CVideo* video = (ui::CVideo*)EngineUI.getWidget("eon_splash_screen");
    if(video->getVideoParams()->video->hasFinished())
        CEngine::get().getModuleManager().changeToGamestate("main_menu");
}
