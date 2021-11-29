#pragma once
#include "modules/module.h"
#include "ui/controllers/ui_menu_controller.h"

namespace input { class CModule; }

class ModuleEONMainMenu : public IModule
{
public:
	ModuleEONMainMenu(const std::string& name) : IModule(name) {}

	bool start() override;
	void stop() override;
	void update(float dt) override;

private:
	void onNewGame();
	void onSettings();
	void onCredits();
	void onExit();

	bool _toSettings = false;

	input::CModule* input = nullptr;
	ui::CMenuController _menuController;
};