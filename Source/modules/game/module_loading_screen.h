#pragma once
#include "modules/module.h"
#include "ui/controllers/ui_menu_controller.h"

namespace input { class CModule; }

class ModuleEONLoadingScreen : public IModule
{
public:
	ModuleEONLoadingScreen(const std::string& name) : IModule(name) {}

	bool start() override;
	void stop() override;
	void update(float delta) override;

private:

	bool _loaded = false;
	bool _ready = false;
	float _timer = 0.f;
	float _discSpeed = 1.f;
	float _discTimer = 0.f;
	input::CModule* input = nullptr;
	ui::CMenuController _menuController;

	void onPlay();
};