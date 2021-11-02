#pragma once
#include "modules/module.h"

namespace input { class CModule; }

class ModuleEONGameplay : public IModule
{
	bool started = false;

public:
	ModuleEONGameplay(const std::string& name) : IModule(name) {}

	bool start() override;
	void stop() override;
	void update(float dt) override;
};