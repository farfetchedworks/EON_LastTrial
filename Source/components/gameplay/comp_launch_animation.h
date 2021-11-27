#pragma once
#include "components/common/comp_base.h"
#include "entity/entity.h"
#include "components/messages.h"

class TCompLaunchAnimation: public TCompBase {

	DECL_SIBLING_ACCESS();

private:

	std::string animation_variable;
	std::string new_group;

	int variable_value = 0;
	float acceptance_dist = 0.f;
	float acceptance_angle = 360.f;

	VEC2 Xlimits = VEC2::Zero;
	VEC2 Zlimits = VEC2::Zero;
	VEC3 offset;
	QUAT targetRotation;

	float lerp_time = 0.5f;
	float timer = 0.f;

	bool enabled = false;

	// Settings
	bool useLimits = false;
	bool useVariableValue = false;
	bool useFinalPose = false;
	bool useInteractCam = true;
	bool skipFront = false;
	bool rotateToFace = false;

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

	VEC3 getOffset() { return offset; }
};
