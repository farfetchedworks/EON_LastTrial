#pragma once
#include "components/common/comp_base.h"
#include "entity/entity.h"
#include "components/messages.h"

class TCompEter : public TCompBase {

	DECL_SIBLING_ACCESS();
	CHandle h_transform;

	VEC3 _targetPosition;

public:

	void onEntityCreated();
	void update(float dt);

	void onHit();
};
