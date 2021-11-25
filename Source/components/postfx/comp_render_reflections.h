#pragma once
#include "comp_bilateral_blur.h"
#include "components/messages.h"

class CRenderToTexture;
class CTexture;

// ------------------------------------
class TCompSSReflections : public TCompBilateralBlur
{
	DECL_SIBLING_ACCESS();

	CRenderToTexture* rt_output = nullptr;
	bool  enabled = true;
	int	idx = 0;

	std::function<CTexture* (CTexture*, CTexture*)> apply_fn;

public:

	void load(const json& j, TEntityParseContext& ctx);
	void onEntityCreated();
	void debugInMenu();
	CTexture* compute(CTexture* in_texture, CTexture* last_texture);
	void setEnabled(bool v) { enabled = v; };

	// ------------- render in fx stack -------------
	void onRender(const TMsgRenderPostFX& msg);
	static void registerMsgs() {
		DECL_MSG(TCompSSReflections, TMsgRenderPostFX, onRender);
	}
};
