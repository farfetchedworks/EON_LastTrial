#pragma once
#include "components/common/comp_base.h"
#include "components/messages.h"

class CRenderToTexture;
class CTexture;

class TCompBilateralBlur : public TCompBase
{
protected:

	int idx = 0;

	CShaderCte< CtesBlur >         cte_blur;

	int iterations = 2;
	float x_offset = 1.f;
	float y_offset = 1.f;
	bool enabled = true;

	CRenderToTexture* rt_half_y = nullptr;
	CRenderToTexture* rt_output = nullptr;

	std::function<CTexture* (CTexture*, CTexture*)> apply_fn;

public:

	TCompBilateralBlur();
	~TCompBilateralBlur();

	void load(const json& j, TEntityParseContext& ctx);
	void onEntityCreated();
	void debugInMenu();

	CTexture* apply(CTexture* in_texture, CTexture* last_texture);

	// ------------- render in fx stack -------------
	void onRender(const TMsgRenderPostFX& msg);
	static void registerMsgs() {
		DECL_MSG(TCompBilateralBlur, TMsgRenderPostFX, onRender);
	}
};
