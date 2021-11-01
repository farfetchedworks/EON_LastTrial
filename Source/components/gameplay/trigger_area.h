#pragma once
#include "entity/entity.h"
#include "components/messages.h"

class ITriggerArea {

protected:
	std::string _name;

public: 

	ITriggerArea(const std::string& name) : _name(name) {}

	virtual void onAreaEnter(CHandle event_trigger, CHandle observer) {};
	virtual void onAreaExit(CHandle event_trigger, CHandle observer) {};

	const std::string& getName() { return _name; }
	CHandle getAreaTrigger(CHandle event_trigger);
};
