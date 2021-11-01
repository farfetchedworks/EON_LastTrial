#pragma once
#include "components/common/comp_base.h"
#include "entity/entity.h"
#include "components/messages.h"

class TCompLaunchAnimation: public TCompBase {

	DECL_SIBLING_ACCESS();

private:

	std::string animation_variable;
	float acceptance_dist = 0.f;
	VEC3 offset;

	float lerp_time = 0.5f;
	float timer = 0.f;
	bool enabled = false;

	// Ideal transform when launching the animation
	CTransform finalPose;

public:
	
	void load(const json& j, TEntityParseContext& ctx);
	void update(float dt);
	void debugInMenu();
	void renderDebug();
	bool resolve();
	void launch();
	void oncomplete();
};
