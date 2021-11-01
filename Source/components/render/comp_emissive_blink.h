#pragma once
#include "components/common/comp_base.h"
#include "components/messages.h"

class TCompEmissiveBlink: public TCompBase
{
	DECL_SIBLING_ACCESS();

	std::string _chars;
	float _unitPeriod = 0.2f;
	float _timer = 0.f;
	float _originalEmissiveValue = 1.f;
	float _emissiveValue = 1.f;
	int _currentChar = 0;

	bool _randomize = false;
	bool _useOriginal = true;

public:
	void load(const json& j, TEntityParseContext& ctx);
	void onEntityCreated();
	void update(float dt);
	void updateEmissive();
};
