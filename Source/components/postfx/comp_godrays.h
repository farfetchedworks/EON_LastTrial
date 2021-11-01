#pragma once
#include "comp_bilateral_blur.h"
class CRenderToTexture;
class CPipelineState;

// ------------------------------------
struct TCompGodRays : public TCompBilateralBlur
{
	CRenderToTexture* rt_volumetric_light = nullptr;

	void load(const json& j, TEntityParseContext& ctx);
	void onEntityCreated();
	void debugInMenu();

	CTexture* generateVolumetricShape(CTexture* in_texture, CTexture* last_texture);

	// ------------- render in fx stack -------------
	void onRender(const TMsgRenderPostFX& msg);
	static void registerMsgs() {
		DECL_MSG(TCompGodRays, TMsgRenderPostFX, onRender);
	}
};