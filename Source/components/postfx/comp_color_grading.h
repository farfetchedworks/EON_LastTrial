#pragma once

#include "components/common/comp_base.h"

class CTexture;
class CRenderToTexture;

// ------------------------------------
struct TCompColorGrading : public TCompBase
{
	bool                          enabled = true;
	bool                          state	= true; // in (true), out (false)

	float                         amount = 1.f;
	float                         timer = 0.f;
	float                         fadeTime = 0.f;
	
	const interpolators::IInterpolator* interpolator = nullptr;

	// lookup table, translates rgb to rgb's
	const CTexture* lut = nullptr;

	void load(const json& j, TEntityParseContext& ctx);
	void update(float dt);
	void debugInMenu();
	
	void fadeIn(float time, const interpolators::IInterpolator* inter = nullptr);
	void fadeOut(float time, const interpolators::IInterpolator* inter = nullptr);

	float getAmount() const {
		return enabled ? amount : 0.f;
	}

};

