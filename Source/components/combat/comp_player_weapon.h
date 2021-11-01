#pragma once

#include "components/common/comp_base.h"
#include "entity/entity.h"
#include "components/messages.h"
#include "components/controllers/pawn_actions.h"
#include "components/combat/comp_weapon.h"

#define NUM_ATTACKS 7

class TCompPlayerWeapon : public TCompWeapon {

	DECL_SIBLING_ACCESS();

private:

	CHandle h_blade_attached_to_bone;
	CHandle h_blade_weapon_trigger;

	int damages[NUM_ATTACKS];
	int current_action = 0;

	int getDamage(bool fromBack);
	void processBackStab(CHandle dst);

public:

	void onEntityCreated();
	void load(const json& j, TEntityParseContext& ctx);
	void update(float dt);
	void debugInMenu();

	void processDamage(CHandle src, CHandle dst);

	void onSetActive(const TMsgEnableWeapon& msg);
	void onTriggerEnter(const TMsgEntityTriggerEnter& msg);
	void onApplyAreaDelay(const TMsgWeaponInAreaDelay& msg);

	int getDamageByHit(int hit_type) { return damages[hit_type]; }

	static void registerMsgs() {
		DECL_MSG(TCompPlayerWeapon, TMsgEnableWeapon, onSetActive);
		DECL_MSG(TCompPlayerWeapon, TMsgEntityTriggerEnter, onTriggerEnter);
		DECL_MSG(TCompPlayerWeapon, TMsgWeaponInAreaDelay, onApplyAreaDelay);
	}
};
