#pragma once

#include "components/common/comp_base.h"
#include "entity/entity.h"
#include "components/messages.h"

class TCompTriggerButton : public TCompBase {

	DECL_SIBLING_ACCESS();

private:

	float current_time = 0.f;									
	bool is_activated = false;								// set to true once the door is opening
	bool is_completely_open = false;					// set to true when the door is completely open
	bool is_in_target = false;								// set to true when the door reached the target or the interpolator is >1

	CHandle target_to_activate;								// Handle of the entity that is activated by the button


	// Called when the associated button is activated
	void onPlayerPress(const TMsgEntityTriggerEnter& msg);
	void onEonInteracted(const TMsgEonInteracted& msg);

public:

	static void registerMsgs() {
		DECL_MSG(TCompTriggerButton, TMsgEntityTriggerEnter, onPlayerPress);
		DECL_MSG(TCompTriggerButton, TMsgEonInteracted, onEonInteracted);
	}

	// Used by the spawner to define itself as the target to activate
	void setParameters(CHandle target);

	void onEntityCreated();
	void update(float dt);
	void debugInMenu();
	void renderDebug();
	void load(const json& j, TEntityParseContext& ctx);
};
