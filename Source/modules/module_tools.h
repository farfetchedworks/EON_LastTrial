#pragma once
#include "modules/module.h"

struct TEntityPickerContext;

class CModuleTools : public IModule {

	CHandle picked_entity;
	bool picked_panel_opened = false;

public:
	CModuleTools(const std::string& name) : IModule(name) { }

	void renderInMenu() override;
	void update(float dt) override;
	bool start() override;
	void stop() override;

	bool pickEntity(TEntityPickerContext &ctx);
};
