#pragma once

#include "mcv_platform.h"

enum class EPhysMatType {
	MARBLE,
	ROCK,
	GRAVEL,
	GRASS,
	WATER
};

class CPhysicalMaterial {
public:
	EPhysMatType type;

	CPhysicalMaterial() : type(EPhysMatType::GRAVEL) {};
	CPhysicalMaterial(const std::string& in_type);

	const char* getName() const;
};