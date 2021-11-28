#pragma once
#include "comp_base.h"

// ----------------------------------------
struct TCompLod : public TCompBase {
	std::string replacement_prefab;
	float       threshold = 250000.0f;

	void load(const json& j, TEntityParseContext& ctx) {
		parse(j);
	}

	void parse(const json& j) {
		if (!j.is_object()) {
			return;
		}

		threshold = j.value("threshold", threshold);
		replacement_prefab = j.value("replacement_prefab", replacement_prefab);
	}
};