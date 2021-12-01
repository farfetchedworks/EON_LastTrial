#pragma once

#include "comp_bilateral_blur.h"

// ------------------------------------
class TCompFocus : public TCompBilateralBlur
{
	int idx = 0;

	bool                   enabled = true;
	CRenderToTexture* rt = nullptr;
	const CPipelineState* pipeline = nullptr;
	const CMesh* mesh = nullptr;
	CShaderCte<CtesFocus>  cte_focus;

	std::function<CTexture* (CTexture*, CTexture*)> apply_fn;

public:
	TCompFocus();
	~TCompFocus();
	void load(const json& j, TEntityParseContext& ctx);
	void onEntityCreated();
	void debugInMenu();

	void enable();
	void disable();

	CShaderCte<CtesFocus>& getCtesFocus();

	CTexture* apply(CTexture* blur_texture, CTexture* last_texture);

	// ------------- render in fx stack -------------
	void onRender(const TMsgRenderPostFX& msg);
	static void registerMsgs() {
		DECL_MSG(TCompFocus, TMsgRenderPostFX, onRender);
	}
};
