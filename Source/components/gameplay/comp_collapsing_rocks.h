#pragma once

#include "components/common/comp_base.h"
#include "entity/entity.h"
#include "components/messages.h"

class TCompCollapsingRocks : public TCompBase {

	DECL_SIBLING_ACCESS();

private:
	std::string spawn_prefab;
	float fall_time = 0.f;

	void onPlayerStepOn(const TMsgEntityTriggerEnter& msg);

public:
	static void registerMsgs() {
		DECL_MSG(TCompCollapsingRocks, TMsgEntityTriggerEnter, onPlayerStepOn);
	}

	void load(const json& j, TEntityParseContext& ctx);
	void debugInMenu();
};
