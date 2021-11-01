#pragma once
#include "modules/module.h"
#include "ui/controllers/ui_menu_controller.h"
#include "audio/module_audio.h"

namespace input { class CModule; }

class ModuleSubvertMainMenu : public IModule
{
  public:
      ModuleSubvertMainMenu(const std::string& name) : IModule(name) {}

    bool start() override;
    void stop() override;
    void update(float dt) override;

  private:
    void onNewGame();
    void onExit();

    input::CModule* _player1 = nullptr;
    ui::CMenuController _menuController;
};