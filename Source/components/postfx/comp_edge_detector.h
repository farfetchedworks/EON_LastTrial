#pragma once
#include "components/messages.h"
#include "components/common/comp_base.h"

class CRenderToTexture;
class CPipelineState;
class CMesh;
class CTexture;

// ------------------------------------
class TCompEdgeDetector : public TCompBase
{
	int idx = 0;

	CRenderToTexture* rt = nullptr;
	const CPipelineState* pipeline = nullptr;
	const CMesh* mesh = nullptr;
	bool enabled = true;

	std::function<CTexture*(CTexture*, CTexture*)> apply_fn;

public:
	void load(const json& j, TEntityParseContext& ctx);
	void onEntityCreated();
	void debugInMenu();

	CTexture* apply(CTexture* in_texture, CTexture* last_texture);

	// ------------- render in fx stack -------------
	void onRender(const TMsgRenderPostFX& msg);
	static void registerMsgs() {
		DECL_MSG(TCompEdgeDetector, TMsgRenderPostFX, onRender);
	}
};
