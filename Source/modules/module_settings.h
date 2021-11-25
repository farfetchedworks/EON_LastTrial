#pragma once
#include "modules/module.h"
#include "ui/controllers/ui_menu_controller.h"

namespace input { class CModule; }

struct TSetting {
	std::string name;
	bool enabled = true;
	std::function<void(bool)> callback = nullptr;
};

class CModuleSettings : public IModule
{
public:
	CModuleSettings(const std::string& name) : IModule(name) {}

	bool start() override;
	void update(float dt) override;
	void stop() override;

	void initSettings();
	void loadDefaultSettings();
	TSetting* getSetting(const std::string& settingName);

private:
	void onChangeSetting(const std::string& buttonName, bool enabled);
	void onGoBack();

	input::CModule* input = nullptr;
	ui::CMenuController _menuController;
	std::vector<TSetting> _settings;
};