#pragma once
#include "modules/module.h"

class ModuleSubvertSplashScreen : public IModule
{
public:
	ModuleSubvertSplashScreen(const std::string& name) : IModule(name) {}

	bool start() override;
	void stop() override;
	void update(float delta) override;

private:
	float _timer = 0.f;
};