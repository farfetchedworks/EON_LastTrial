#pragma once

#include "modules/module.h"

namespace input { class CModule; }

class ModuleSubvertGameplay : public IModule
{
	bool started = false;

public:
	ModuleSubvertGameplay(const std::string& name) : IModule(name) {}

	bool start() override;
	void stop() override;
	void update(float dt) override;
};