#pragma once

#include "components/common/comp_base.h"
#include "entity/entity.h"
#include "bone_correction.h"

class CalModel;
struct TCompSkeleton;

struct TCompSkelLookAt : public TCompBase {

	DECL_SIBLING_ACCESS();

	CHandle     h_skeleton;        // Handle to comp_skeleton of the entity being tracked
	CHandle     h_target_entity;

	std::string target_entity_name;

	VEC3        target;
	VEC3        new_target;
	VEC3        offset;
	float       amount = 0.f;
	float       target_transition_factor = 0.95f;
	bool        look_at_player = false;
	std::string	head_bone_name = std::string();

	void load(const json& j, TEntityParseContext& ctx);
	void update(float dt);
	void debugInMenu();
	void renderDebug();

	void setTarget(const VEC3& target_pos);
	void setEnabled(bool v) { enabled = v; }
	bool isLookingAtTarget(float accept_dist = 0.1f);

private:

	bool        enabled = true;
};
