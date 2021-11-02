#pragma once
#include "modules/module.h"

class ModuleEONSplashScreen : public IModule
{
public:
	ModuleEONSplashScreen(const std::string& name) : IModule(name) {}

	bool start() override;
	void stop() override;
	void update(float delta) override;

private:
	float _timer = 0.f;
};