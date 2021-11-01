#pragma once

#include "components/common/comp_base.h"
#include "entity/entity.h"
#include "components/messages.h"
#include "components/combat/comp_weapon.h"

class TCompEnemyWeapon : public TCompWeapon {

	DECL_SIBLING_ACCESS();

private:
	
	int current_damage = 0;
	std::string hit_type;

public:

	void onEntityCreated();
	void load(const json& j, TEntityParseContext& ctx);
	void update(float dt);
	void debugInMenu();

	void processDamage(CHandle src, CHandle dst);

	void onSetActive(const TMsgEnableWeapon& msg);
	void onTriggerEnter(const TMsgEntityTriggerEnter& msg);
	void onApplyAreaDelay(const TMsgWeaponInAreaDelay& msg);

	static void registerMsgs() {
		DECL_MSG(TCompEnemyWeapon, TMsgEnableWeapon, onSetActive);
		DECL_MSG(TCompEnemyWeapon, TMsgEntityTriggerEnter, onTriggerEnter);
		DECL_MSG(TCompEnemyWeapon, TMsgWeaponInAreaDelay, onApplyAreaDelay);
	}
};
