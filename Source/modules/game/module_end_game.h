#pragma once

#include "modules/module.h"
#include "ui/controllers/ui_menu_controller.h"
#include "audio/module_audio.h"

namespace input { class CModule; }

class ModuleEONEndGame : public IModule
{
public:
    ModuleEONEndGame(const std::string& name) : IModule(name) {}

    bool start() override;
    void stop() override;
    void update(float dt) override;
    void onContinue();
    void onExit();

private:

    input::CModule* input = nullptr;
    ui::CMenuController _menuController;
};