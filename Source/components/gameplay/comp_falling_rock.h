#pragma once
#include "components/common/comp_base.h"
#include "entity/entity.h"
#include "components/messages.h"

class TCompFallingRock : public TCompBase {

	DECL_SIBLING_ACCESS();

	float _waitTime = 0.f;
	float _timer = 0.f;
	bool _isFalling = false;

public:

	void onEntityCreated();
	void setWaitTime(float time) { _waitTime = time; };
	void update(float dt);
};
