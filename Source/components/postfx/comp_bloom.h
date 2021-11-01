#pragma once
#include "components/common/comp_base.h"
#include "components/messages.h"

class CRenderToTexture;
class CPipelineState;

// ------------------------------------
struct TCompBloom : public TCompBase
{
	int idx = 0;

	CShaderCte< CtesBloom > cte_bloom;

	const CPipelineState*	pipeline_filter = nullptr;
	const CPipelineState*	pipeline_scale = nullptr;
	const CPipelineState*	pipeline_add = nullptr;
	const CMesh*			mesh = nullptr;

	bool					enabled = true;
	float					multiplier = 1.0f;
	int						iterations = 4;
	float					threshold_min = 1.f;
	float					delta = 1.f;
	float					persistence = 0.5f;

	TCompBloom();
	~TCompBloom();

	std::function<CTexture* (CTexture*, CTexture*)> apply_fn;

	void load(const json& j, TEntityParseContext& ctx);
	void onEntityCreated();
	void debugInMenu();

	unsigned int textures_created = 0;
	std::vector<CRenderToTexture*>	texture_pool;
	CRenderToTexture* getTemporary(int width, int height);
	void releaseTemporary(CRenderToTexture* texture);
	CTexture* generateHighlights(CTexture* in_texture, CTexture* last_texture);

	// ------------- render in fx stack -------------
	void onRender(const TMsgRenderPostFX& msg);
	static void registerMsgs() {
		DECL_MSG(TCompBloom, TMsgRenderPostFX, onRender);
	}
};