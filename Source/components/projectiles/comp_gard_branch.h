#pragma once

#include "components/common/comp_base.h"
#include "entity/entity.h"
#include "components/messages.h"

namespace FMOD {
	namespace Studio {
		class EventInstance;
	}
}

class TCompGardBranch : public TCompBase {

	DECL_SIBLING_ACCESS();

private:

	int damage = 10;
	
	CHandle h_branch_trans;
	CHandle h_branch_collider;

	// Stored audio event, to kill whenever the branch destroys itself
	FMOD::Studio::EventInstance* fmod_event = nullptr;

	// Called when the branch has hit anything	
	void onHitObject(const TMsgEntityOnContact& msg);

public:

	static void registerMsgs() {
		DECL_MSG(TCompGardBranch, TMsgEntityOnContact, onHitObject);
	}

	void setParameters(int new_damage);

	void load(const json& j, TEntityParseContext& ctx);
	void onEntityCreated();
	void update(float dt);
	void debugInMenu();
	void destroy();
};
