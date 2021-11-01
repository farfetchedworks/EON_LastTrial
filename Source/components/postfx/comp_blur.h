#pragma once

#include "components/common/comp_base.h"
#include "components/messages.h"

class CRenderToTexture;
class CTexture;
class CBlurStep;

class TCompBlur : public TCompBase
{

protected:

	int idx = 0;

	std::vector< CBlurStep* > steps;
	VEC4       weights;
	VEC4       distance_factors;    // 1,2,3,XX
	float      global_distance = 1.0f;
	int        nactive_steps = 0;
	bool       enabled = true;

	std::function<CTexture* (CTexture*, CTexture*)> apply_fn;

	void  setPreset1();
	void  setPreset2();

public:
	void  load(const json& j, TEntityParseContext& ctx);
	void onEntityCreated();
	void  debugInMenu();

	CTexture* apply(CTexture* in_texture, CTexture* last_texture);

	// ------------- render in fx stack -------------
	void onRender(const TMsgRenderPostFX& msg);
	static void registerMsgs() {
		DECL_MSG(TCompBlur, TMsgRenderPostFX, onRender);
	}
};
