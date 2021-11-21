#include "mcv_platform.h"
#include "module_loading_screen.h"
#include "engine.h"
#include "modules/module_manager.h"
#include "modules/module_boot.h"
#include "render/render_module.h"
#include "input/input_module.h"
#include "ui/ui_module.h"
#include "ui/ui_widget.h"
#include "fmod_studio.hpp"
#include "audio/module_audio.h"
#include "ui/widgets/ui_image.h"
#include "ui/widgets/ui_button.h"

bool ModuleEONLoadingScreen::start()
{
    input = CEngine::get().getInput(input::MENU);
    assert(input);

    _discSpeed = 1.f;
    _timer = 2.f; // sync with title screen blend out
    EngineRender.setClearColor({0.f, 0.f, 0.f, 1.f});

    // Detect input and set respective texture

    ui::CWidget* w = EngineUI.getWidget("eon_loading_screen");
    assert(w);
    ui::CImage* img = static_cast<ui::CImage*>(w);
    ui::TImageParams& params = img->imageParams;

    params.texture = Resources.get( PlayerInput.getPad().connected ?
        "data/textures/ui/subvert/loading/loading_gamepad.dds" : 
        "data/textures/ui/subvert/loading/loading_keyboard.dds" )->as<CTexture>();
    EngineUI.activateWidget("eon_loading_screen");

    _menuController.setInput(input);
    _menuController.bind("continue_btn_loading", std::bind(&ModuleEONLoadingScreen::onPlay, this));
    _menuController.reset();

    return true;
}

void ModuleEONLoadingScreen::stop()
{
    EngineUI.deactivateWidget("eon_loading_screen");
    EngineAudio.setMusicRTPC("End_Theme", 1);
    PlayerInput.unBlockInput();
}

void ModuleEONLoadingScreen::update(float dt)
{
    _timer -= dt;

    ui::CWidget* w = EngineUI.getWidget("disc");
    assert(w);
    _discSpeed = damp(_discSpeed, _ready ? 0.f : 1.f, 2.f, dt);
    w->setAngle(w->getAngle() + dt * _discSpeed);

    if (!_loaded && _timer < 0.f)
    {
        Boot.customStart();
        _loaded = true;
    }

    if (!_ready && Boot.ready())
    {
        if (!Boot.inGame())
        {
            onPlay();
            return;
        }

        _ready = true;
        _menuController.selectOption(0);

        ui::CWidget* w = EngineUI.getWidget("continue_btn_loading");
        assert(w);
        w->setVisible(true);

        w = EngineUI.getWidget("eon_loading_screen");
        assert(w);
        ui::CImage* img = static_cast<ui::CImage*>(w);
        ui::TImageParams& params = img->imageParams;
        params.texture = Resources.get(PlayerInput.getPad().connected ?
            "data/textures/ui/subvert/loading/loading_gamepad_no_text.dds" :
            "data/textures/ui/subvert/loading/loading_keyboard_no_text.dds")->as<CTexture>();

        CApplication::get().changeMouseState(true, false);
    }

    if (_ready)
    {
        _menuController.update(dt);
    }
}

void ModuleEONLoadingScreen::onPlay()
{
    CEngine::get().getModuleManager().changeToGamestate("playing");
}
