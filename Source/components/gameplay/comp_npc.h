#pragma once
#include "components/common/comp_base.h"
#include "entity/entity.h"
#include "components/messages.h"

class TCompNPC : public TCompBase {

	DECL_SIBLING_ACCESS();

public:
	
	void load(const json& j, TEntityParseContext& ctx);
	void update(float dt);

	bool resolve();
	void interact();

	void onStop(const TMsgStopCaption& msg);

	static void registerMsgs() {
		DECL_MSG(TCompNPC, TMsgStopCaption, onStop);
	}
};
