#pragma once

#include "components/common/comp_base.h"
#include "entity/entity.h"
#include "components/messages.h"

class TCompDestructible : public TCompBase {

	DECL_SIBLING_ACCESS();

private:
	CHandle h_transform; // cached transform component

	bool drops_warp = true;

	std::string spawn_prefab;
	std::string fmod_event; // path to the event to play when the prop is destroyed
	void onDestroy(const TMsgPropDestroyed& msg);

public:
	static void registerMsgs() {
		DECL_MSG(TCompDestructible, TMsgPropDestroyed, onDestroy);
	}

	void load(const json& j, TEntityParseContext& ctx);
	void onEntityCreated();
	void debugInMenu();
};
