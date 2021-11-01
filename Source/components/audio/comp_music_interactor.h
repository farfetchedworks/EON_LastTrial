#pragma once

#include "components/common/comp_base.h"
#include "entity/entity.h"
#include "components/messages.h"

class TCompMusicInteractor : public TCompBase {

	DECL_SIBLING_ACCESS();

private:
	bool enabled = false; // We want it to be disabled by default, until we reach an area/boss that requires it

	int targeted_count = 0;

	float min_monk_distance_SQ = 225.f;
	float min_HP = 0.3f;

	CHandle h_cache_transform;
	CHandle h_cache_health;

public:

	void onEntityCreated();
	void load(const json& j, TEntityParseContext& ctx);
	void update(float dt);
	void debugInMenu();

	void setEnabled(const bool val) { enabled = val; };

	void onTargeted(const TMsgTarget& msg);
	void onUntargeted(const TMsgUntarget& msg);

	static void registerMsgs() {
		DECL_MSG(TCompMusicInteractor, TMsgTarget, onTargeted);
		DECL_MSG(TCompMusicInteractor, TMsgUntarget, onUntargeted);
	}
};
