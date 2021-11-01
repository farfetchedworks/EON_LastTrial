#pragma once
#include "components/common/comp_base.h"
#include "components/messages.h"

class CTexture;
class CRenderToTexture;

struct TCompDepthFog : public TCompBase
{
	bool  enabled = true;
	int idx = 0;

	CShaderCte< CtesDepthFog > ctes;

	CRenderToTexture* rt_fog = nullptr;
	std::function<CTexture* (CTexture*, CTexture*)> apply_fn;

	TCompDepthFog();
	~TCompDepthFog();
	void load(const json& j, TEntityParseContext& ctx);
	void onEntityCreated();
	void debugInMenu();

	CTexture* renderFog(CTexture* in_texture, CTexture* last_texture);

	// ------------- render in fx stack -------------
	void onRender(const TMsgRenderPostFX& msg);
	static void registerMsgs() {
		DECL_MSG(TCompDepthFog, TMsgRenderPostFX, onRender);
	}
};

