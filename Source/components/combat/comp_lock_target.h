#pragma once

#include "entity/entity.h"

class TCompLockTarget : public TCompBase {

protected:

	std::string bone_name;
	int bone_id;
	CHandle h_target_skeleton;

	float height = -1.f;

public:

	void onEntityCreated();
	void load(const json& j, TEntityParseContext& ctx);
	void debugInMenu();
	
	bool resolveTarget();
	VEC3 getPosition();
	float getHeight() const { return height; }
};
