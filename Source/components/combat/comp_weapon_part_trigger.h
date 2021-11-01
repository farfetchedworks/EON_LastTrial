#pragma once

#include "components/common/comp_base.h"
#include "entity/entity.h"
#include "components/messages.h"
#include "components/controllers/pawn_actions.h"
#include "components/common/comp_transform.h"

class TCompWeaponPartTrigger : public TCompBase {

	DECL_SIBLING_ACCESS();

private:

	CHandle h_floor_parts;
	CHandle parent;
	CHandle h_hierarchy;
	CHandle h_attached_to_bone;
	int current_action = 0;

	VEC3 offset = VEC3::Zero;
	std::deque<VEC3> knots; // Used for CatMullRom

	float frames = 0.f;

	// Enabled if state is in active frames
	bool enabled = false;
	bool is_point = false; // The point marks the end of the blade

public:

	void load(const json& j, TEntityParseContext& ctx);
	void onEntityCreated();
	void update(float dt);
	void onSetActive(const TMsgEnableWeapon& msg);
	void onTriggerEnter(const TMsgEntityTriggerEnter& msg);
	void debugInMenu();
	void renderDebug();

	VEC3 getOffset() { return offset; }
	void spawnFloorParticles();

	static void registerMsgs() {
		DECL_MSG(TCompWeaponPartTrigger, TMsgEnableWeapon, onSetActive);
		DECL_MSG(TCompWeaponPartTrigger, TMsgEntityTriggerEnter, onTriggerEnter);
	}
};
