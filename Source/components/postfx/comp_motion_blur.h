#pragma once
#include "components/common/comp_base.h"
#include "render/render.h"
#include "components/messages.h"

class CRenderToTexture;
class CTexture;
class CMesh;
class CPipelineState;

// ------------------------------------
class TCompMotionBlur : public TCompBase
{
	int idx = 0;

	bool                   enabled = true;
	CRenderToTexture* rt = nullptr;
	const CPipelineState* pipeline = nullptr;
	const CMesh* mesh = nullptr;
	CShaderCte<CtesMotionBlur>  ctes;
	std::function<CTexture* (CTexture*, CTexture*)> apply_fn;

	float timer = -1.f;
	float blend_in = 0.5f;
	float blend_out = 1.5f;

	const interpolators::IInterpolator* fadein = nullptr;
	const interpolators::IInterpolator* fadeout = nullptr;

public:
	TCompMotionBlur();
	~TCompMotionBlur();
	void load(const json& j, TEntityParseContext& ctx);
	void onEntityCreated();
	void debugInMenu();

	void enable();
	void disable();
	bool isEnabled() { return enabled; };

	CTexture* apply(CTexture* input_texture, CTexture* last_texture);

	// ------------- render in fx stack -------------
	void onRender(const TMsgRenderPostFX& msg);
	static void registerMsgs() {
		DECL_MSG(TCompMotionBlur, TMsgRenderPostFX, onRender);
	}
};
