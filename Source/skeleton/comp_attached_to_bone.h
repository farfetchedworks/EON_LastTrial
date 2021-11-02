#pragma once

#include "components/common/comp_base.h"
#include "components/common/comp_transform.h"
#include "entity/entity.h"

struct TCompAttachedToBone : public TCompBase {

	DECL_SIBLING_ACCESS();

	CHandle			h_target_skeleton;    // TCompSkeleton
	CHandle			h_my_transform;
	int				bone_id = -1;
	std::string		target_entity_name;
	std::string		bone_name;
	VEC3			offset = VEC3::Zero;
	VEC3			offset_rotation = VEC3::Zero;

	CTransform		local_t;
	bool			skip_rotation = false;
	bool			enabled = true;

	bool resolveTarget(CHandle h_entity_parent);
	void load(const json& j, TEntityParseContext& ctx);
	void update(float);
	void applyOffset(CTransform& trans, VEC3 new_offset);
	void debugInMenu();
	void onEntityCreated();

	void detach() { enabled = false; };
	void attach() { enabled = true; };
};

