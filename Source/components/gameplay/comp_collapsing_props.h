#pragma once

#include "components/common/comp_base.h"
#include "entity/entity.h"
#include "components/messages.h"

class TCompCollapsingProps : public TCompBase {

	DECL_SIBLING_ACCESS();

private:
	std::string spawn_prefab;
	std::string fmod_event;
	std::string type;
	float fall_time = 0.f;

	void processBridge(TEntityParseContext& ctx);
	void processRocks(TEntityParseContext& ctx);
	void onPlayerStepOn(const TMsgEntityTriggerEnter& msg);

public:
	static void registerMsgs() {
		DECL_MSG(TCompCollapsingProps, TMsgEntityTriggerEnter, onPlayerStepOn);
	}

	void load(const json& j, TEntityParseContext& ctx);
	void debugInMenu();
};
