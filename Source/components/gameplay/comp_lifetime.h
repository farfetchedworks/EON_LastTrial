#pragma once

#include "mcv_platform.h"
#include "components/common/comp_base.h"

class TCompLifetime : public TCompBase {

	DECL_SIBLING_ACCESS();

private:
	float _ttl = 0.f;
	float _timer = 0.f;
	bool _enabled = true;
	bool _executed = false;

public:

	void load(const json& j, TEntityParseContext& ctx);
	void setTTL(float lifespan = 0.f) { _ttl = lifespan; }
	void onEntityCreated();
	void init();
};