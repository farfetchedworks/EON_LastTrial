#pragma once

#include "components/common/comp_base.h"
#include "entity/entity.h"

class TCompAreaDelay : public TCompBase {

	DECL_SIBLING_ACCESS();

private:

	int warp_consumption = 10;

	// Area delay
	float area_duration					= 5.f;
	float area_radius					= 5.f;
	float speed_reduction				= 5.f;
	float ad_ball_timer					= 0.f;

	CHandle h_active_wave;
	CHandle h_transform;
	CHandle area_delay_projectile;	// Contains the last area delay projectile that was spawned
	CHandle h_plasma;

	bool is_casted = false;
	bool ad_ball = false;
	bool is_ball_fading = false;

	VEC3 next_area_position;
	const interpolators::IInterpolator* in_interpolator = nullptr;
	const interpolators::IInterpolator* out_interpolator = nullptr;

public:

	void onEntityCreated();
	void load(const json& j, TEntityParseContext& ctx);
	void update(float dt);
	void debugInMenu();

	// Instantiate an area delay in a specific position
	bool startAreaDelay(CHandle locked_t);																						// For Eon
	bool startAreaDelayEnemy(CHandle locked_t);																				// For enemies with area delay
	bool startAreaDelay(CHandle locked_t, float duration, bool enemy_caster = false);	// Generic start area delay
	bool startWaveDelay(float initial_radius, float max_radius, float duration, bool byPlayer = false);

	void setADBall(CHandle h) { h_plasma = h; }
	void destroyADBall(bool thrown = true);
	void detachADBall();
	/*
		Returns true if an area delay is already cast in the scene.
		In such case, Eon cannot cast an area delay until the lates projectile is destroyed
	*/
	bool isActive();
	bool isCasted() { return is_casted; }
	int getWarpConsumption() { return warp_consumption; }
};
