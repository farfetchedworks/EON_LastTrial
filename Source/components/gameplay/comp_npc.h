#pragma once
#include "components/common/comp_base.h"
#include "entity/entity.h"
#include "components/messages.h"

class TCompNPC : public TCompBase {

	DECL_SIBLING_ACCESS();

	float sight_radius = 2.f;
	std::string caption_scene;

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
