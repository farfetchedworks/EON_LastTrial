#pragma once

#include "components/common/comp_base.h"
#include "entity/entity.h"
#include "trigger_area.h"
#include "components/messages.h"

class TCompTriggerArea : public TCompBase {

	DECL_SIBLING_ACCESS();

private:

	bool enabled = true;
	bool restrict_eon = true;

	std::string area_name;							// Name of the area to be mapped to events in the trigger area manager
	bool remove_after_enter = false;				// Destroy this component when Eon enters the trigger
	bool remove_after_exit = false;					// Destroy this component when Eon exits the trigger

	// Called when the associated button is activated
	void onTriggerEnter(const TMsgEntityTriggerEnter& msg);
	void onTriggerExit(const TMsgEntityTriggerExit& msg);

	void destroyTrigger();

public:

	CHandle h_hit_object = CHandle();

	static void registerMsgs() {
		DECL_MSG(TCompTriggerArea, TMsgEntityTriggerEnter, onTriggerEnter);
		DECL_MSG(TCompTriggerArea, TMsgEntityTriggerExit, onTriggerExit);
	}

	void load(const json& j, TEntityParseContext& ctx);
	void onEntityCreated();
	void renderDebug();
};
