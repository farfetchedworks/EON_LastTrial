#pragma once

#include "components/common/comp_base.h"
#include "entity/entity.h"
#include "components/messages.h"

class TCompOnOffButton : public TCompBase {

	DECL_SIBLING_ACCESS();

private:

	bool is_deactivated = true;
	bool has_to_be_reset = false;								// If it is true, the state is reset when level is reloaded after Eon's death

	CHandle target_a_to_activate;								// Handle of the entity that is activated by the button
	CHandle target_b_to_activate;								// Handle of another entity that can be activated by the button


	// Called when the associated button is activated
	void onPlayerPress(const TMsgEntityTriggerEnter& msg);

public:

	static void registerMsgs() {
		DECL_MSG(TCompOnOffButton, TMsgEntityTriggerEnter, onPlayerPress);
	}

	// Used by the spawner to define itself as the target to activate
	void setParameters(CHandle target_a, CHandle target_b, bool reset_state);

	// Used by the game manager to restore its state if necessary
	void restoreState();

	void onEntityCreated();
	void update(float dt);
	void debugInMenu();
	void renderDebug();
	void load(const json& j, TEntityParseContext& ctx);
};
