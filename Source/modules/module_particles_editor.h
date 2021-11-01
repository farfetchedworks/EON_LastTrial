#pragma once
#include "modules/module.h"

class CModuleParticlesEditor : public IModule {

	bool opened = false;

public:
	CModuleParticlesEditor(const std::string& name) : IModule(name) { }

	bool start() override;
	void stop() override;
	void update(float dt) override;
	void renderInMenu() override;
	void toggle() { opened = !opened; };
};
