#pragma once
#include "modules/module.h"

class CModuleMultithreading : public IModule {
 
public:
	CModuleMultithreading(const std::string& name) : IModule(name) { }
	bool start() override;
	void stop() override;
	void update(float dt) override;

};