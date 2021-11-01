#pragma once
#include "components/common/comp_base.h"
#include "components/messages.h"

class TCompParticlesFollowTarget : public TCompBase
{
	DECL_SIBLING_ACCESS();

	bool _enabled = false;
	CHandle _target;
	VEC3 _offset;

public:

	void resolve(const std::string& target_name, VEC3 offset = VEC3::Zero);
	void update(float dt);
};