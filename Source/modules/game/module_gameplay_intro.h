#pragma once
#include "modules/module.h"

namespace input { class CModule; }

class ModuleEONGameplayIntro : public IModule
{
	float timer = 0.f;
	bool started = false;

public:
	
	ModuleEONGameplayIntro(const std::string& name) : IModule(name) {}

	bool start() override;
	bool customStart();
	void stop() override;
	void update(float dt) override;
};