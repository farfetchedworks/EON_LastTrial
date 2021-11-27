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

	void setCaller(const std::string& screen) { _callingScreen = screen; };
	bool fromCaller(const std::string& screen) { return _callingScreen == screen; };

private:
	void onChangeSetting(const std::string& buttonName, bool enabled);
	void onGoBack();

	bool _toGameplay = false;

	input::CModule* input = nullptr;
	ui::CMenuController _menuController;
	std::vector<TSetting> _settings;
	std::string _callingScreen = "";
};