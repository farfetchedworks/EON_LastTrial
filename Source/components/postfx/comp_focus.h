#pragma once

#include "components/common/comp_base.h"
#include "render/render.h"
#include "components/messages.h"

class CRenderToTexture;
class CTexture;
class CMesh;
class CPipelineState;

// ------------------------------------
class TCompFocus : public TCompBase
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

	CTexture* apply(CTexture* blur_texture, CTexture* last_texture);

	// ------------- render in fx stack -------------
	void onRender(const TMsgRenderPostFX& msg);
	static void registerMsgs() {
		DECL_MSG(TCompFocus, TMsgRenderPostFX, onRender);
	}
};
