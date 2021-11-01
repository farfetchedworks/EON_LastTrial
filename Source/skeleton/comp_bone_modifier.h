#pragma once
#include "components/common/comp_base.h"
#include "entity/entity.h"

struct TCompBoneModifier : public TCompBase {

	DECL_SIBLING_ACCESS();

	int				bone_id = -1;
	std::string		bone_name;
	CHandle			h_skeleton;

	float			scale = 0.f;
	float			frequency = 0.f;

	void load(const json& j, TEntityParseContext& ctx);
	void onEntityCreated();
	bool resolveTarget();
	void updateBone();

};

