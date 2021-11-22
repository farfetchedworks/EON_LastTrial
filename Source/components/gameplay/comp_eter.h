#pragma once
#include "components/common/comp_base.h"
#include "entity/entity.h"
#include "components/messages.h"

class TCompEter : public TCompBase {

	DECL_SIBLING_ACCESS();
	CHandle h_transform;

	bool _isBroken = false;
	bool _spawned = false;
	bool _exploded = false;
	VEC3 _targetPosition;
	VEC3 _initialPosition;

public:

	void load(const json& j, TEntityParseContext& ctx);
	void onEntityCreated();
	void update(float dt);

	void spawnBrokenEter();
	void onHit();
};
