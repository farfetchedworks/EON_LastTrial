#pragma once

#include "entity/entity.h"
#include "components/common/comp_base.h"
#include "components/messages.h"

class TCompWarpEnergy : public TCompBase {

	DECL_SIBLING_ACCESS();

	float warp_energy			= 6;
	int max_warp_energy			= 6;
	int curr_max_warp_energy	= 6;
	float on_hit_amount_warp	= 0.2f;
	float warp_recovery_speed	= 0.05f;
	float empty_warp_timer		= 0.f;

public:

	static void registerMsgs() {
		DECL_MSG(TCompWarpEnergy, TMsgHitWarpRecover, onHit);
	}

	void onHit(const TMsgHitWarpRecover& msg);

	void consumeWarpEnergy(int warp_nrg_points);
	void fillWarpEnergy() { warp_energy = (float)curr_max_warp_energy; };
	void setMaxWarp(int max) { max_warp_energy = max; };
	void setCurrMaxWarp(int max) { curr_max_warp_energy = max; };
	bool hasWarpEnergy(int warp_cost);

	void update(float dt);
	void debugInMenu();
	void load(const json& j, TEntityParseContext& ctx);
};
