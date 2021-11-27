#pragma once
#include "modules/module.h"
#include "entity/entity.h"

namespace input { class CModule; }

class CModulePlayerInteraction : public IModule
{
	CHandle h_player;
	CHandle h_lastShrine;

	CHandle h_currEnergyWall;
	CHandle h_currAnimLauncher;

	bool lastWidgetActive = false;
	bool active = false;

	void enableUI(bool v);

public:
	CModulePlayerInteraction(const std::string& name) : IModule(name) {}

	CHandle getAnimationLauncher() { return h_currAnimLauncher; };
	CHandle getEnergyWall() { return h_currEnergyWall; }
	CHandle getLastShrine() { return h_lastShrine; }

	void setAnimationLauncher(CHandle h) { h_currAnimLauncher = h; };
	void setEnergyWall(CHandle h) { h_currEnergyWall = h; };
	void setLastShrine(CHandle h) { h_lastShrine = h; };
	void setActive(bool v) { active = v; };

	bool start() override;
	void checkInteractions();
	void interact(CHandle object);

	bool isEnergyWallActive();
	bool isLookingAtTheEnergyWall();
};