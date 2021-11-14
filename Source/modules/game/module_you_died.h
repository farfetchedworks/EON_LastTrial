#pragma once
#include "modules/module.h"
#include "ui/controllers/ui_menu_controller.h"
#include "audio/module_audio.h"

namespace input { class CModule; }

class ModuleEONYouDied : public IModule
{
public:
	ModuleEONYouDied(const std::string& name) : IModule(name) {}

	bool start() override;
	void stop() override;
	void update(float dt) override;

private:
	void onContinue();
	void onSurrender();
	void onExit();

	input::CModule* _input = nullptr;
	ui::CMenuController _menuController;
	FMOD::Studio::EventInstance* fmod_event = nullptr;
};