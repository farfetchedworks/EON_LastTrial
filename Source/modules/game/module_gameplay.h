#pragma once
#include "modules/module.h"
#include "ui/controllers/ui_menu_controller.h"

namespace input { class CModule; }

class ModuleEONGameplay : public IModule
{
	bool started = false;
	bool paused = false;
	input::CModule* input = nullptr;
	ui::CMenuController _menuController;

public:
	
	ModuleEONGameplay(const std::string& name) : IModule(name) {}

	bool start() override;
	void stop() override;
	void update(float dt) override;
	void togglePause();

	void onResume();
	void onExit();
};