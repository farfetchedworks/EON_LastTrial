#pragma once

#include "components/common/comp_base.h"
#include "entity/entity.h"

class TCompHeal : public TCompBase {

	DECL_SIBLING_ACCESS();

private:

	int warp_consumption = 4;
	int heal_value = 20;
	CHandle c_health;

public:

	~TCompHeal();
	void load(const json& j, TEntityParseContext& ctx);
	void onEntityCreated();
	void debugInMenu();

	void heal();
	int getWarpConsumption() { return warp_consumption; }
};
