#pragma once
#include "components/common/comp_base.h"
#include "components/messages.h"

class TCompEmissiveMod : public TCompBase
{
	DECL_SIBLING_ACCESS();

	float _minValue = 0.f;
	float _maxValue = 1.f;
	float _emissiveValue = 1.f;

	bool _blendIn = false;
	bool _blendOut = false;
	
	float _inSpeed = 2.f;
	float _outSpeed = 2.f;

public:
	void load(const json& j, TEntityParseContext& ctx);
	void onEntityCreated();
	void update(float dt);
	void updateEmissive();
	void debugInMenu();

	void blendIn();
	void blendOut();
};
