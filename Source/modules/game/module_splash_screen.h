#pragma once
#include "modules/module.h"

namespace FMOD {
	namespace Studio {
		class EventInstance;
	}
}

class ModuleEONSplashScreen : public IModule
{
private:

	FMOD::Studio::EventInstance* fmod_event;

public:
	ModuleEONSplashScreen(const std::string& name) : IModule(name) {}
	bool start() override;
	void stop() override;
	void update(float delta) override;

	input::CModule* input = nullptr;
};