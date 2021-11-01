#pragma once

#include "entity/entity.h"
#include "components/messages.h"

class TCompWeapon : public TCompBase {

protected:

	CHandle parent;
	CHandle owner_weapon;

	int dmg_regular = 0;
	int dmg_heavy = 0;

	float range = 2.f;

	// Enabled if state is in active frames
	bool enabled = false;

	// If the enemy is in area delay, the weapon cannot be enabled (despite being in active frames)
	bool in_area_delay = false;

public:

	virtual void onEntityCreated() = 0;
	virtual void load(const json& j, TEntityParseContext& ctx) = 0;
	virtual void update(float dt) = 0;
	virtual void debugInMenu() = 0;

	bool checkHit(CEntity* eSrc, CEntity* eDst, bool& fromBack);
	void onTriggerEnter(const TMsgEntityTriggerEnter& msg, CHandle h_owner, VEC3 position);
	virtual void processDamage(CHandle src, CHandle dst) = 0;
};
