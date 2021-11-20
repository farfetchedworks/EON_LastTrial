#pragma once
#include "components/common/comp_base.h"
#include "entity/entity.h"
#include "components/messages.h"

class TCompAether : public TCompBase {

	DECL_SIBLING_ACCESS();
	CHandle h_transform;

	VEC3 _finalPose;

public:

	void onEntityCreated();
	void update(float dt);

	void onHit();
};
