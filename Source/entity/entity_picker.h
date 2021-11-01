#pragma once

struct TRay {
	VEC3 origin;
	VEC3 direction;
};

struct TEntityPickerContext {

	bool hasHit = false;

	VEC3 hit_position;
	CHandle current_entity;
	std::string current_name;

	TEntityPickerContext() = default;
};

bool testSceneRay(TRay ray, TEntityPickerContext& ctx);

