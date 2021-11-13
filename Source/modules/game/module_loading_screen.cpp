#include "mcv_platform.h"
#include "module_loading_screen.h"
#include "engine.h"
#include "modules/module_manager.h"
#include "modules/module_boot.h"
#include "render/render_module.h"
#include "input/input_module.h"
#include "ui/ui_module.h"
#include "ui/ui_widget.h"
#include "ui/widgets/ui_image.h"
#include "ui/widgets/ui_button.h"

bool ModuleEONLoadingScreen::start()
{
    input = CEngine::get().getInput(input::MENU);
    assert(input);

    _timer = 2.f; // sync with title screen blend out
    EngineRender.setClearColor({0.f, 0.f, 0.f, 1.f});
    EngineUI.activateWidget("eon_loading_screen");

    _menuController.setInput(input);
    _menuController.bind("continue_btn", std::bind(&ModuleEONLoadingScreen::onPlay, this));
    _menuController.reset();

    return true;
}

void ModuleEONLoadingScreen::stop()
{
    EngineUI.deactivateWidget("eon_loading_screen");
}

void ModuleEONLoadingScreen::update(float dt)
{
    _timer -= dt;

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

        ui::CWidget* w = EngineUI.getWidget("continue_btn");
        assert(w);
        if (w) {
            
            // Change textures of states
            {
                ui::CButton* btn = static_cast<ui::CButton*>(w);
                for (auto& s : btn->states)
                {
                    s.second.imageParams.texture = Resources.get("data/textures/ui/subvert/menu/continue_selected.dds")->as<CTexture>();
                }
            }

            // Change basic texture
            {
                ui::CImage* img = static_cast<ui::CImage*>(w);
                ui::TImageParams& params = img->imageParams;
                params.texture = Resources.get("data/textures/ui/subvert/menu/continue_selected.dds")->as<CTexture>();
            }
        }
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
