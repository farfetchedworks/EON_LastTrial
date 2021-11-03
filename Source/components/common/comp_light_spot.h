#pragma once
#include "components/messages.h"
#include "comp_base.h"
#include "comp_camera.h"
#include "render/textures/render_to_texture.h"

class TCompLightSpot : public TCompCamera {
	DECL_SIBLING_ACCESS();

	void copyZ(CRenderToTexture* src, CRenderToTexture* dst);

public:

	VEC4 color = Colors::White;
	const CTexture* pattern = nullptr;
	CRenderToTexture* shadows_rt = nullptr;
	CRenderToTexture* cached_shadow_map = nullptr;
	bool casts_shadows = false;
	bool cast_godrays = false;
	bool shadows_enabled = false;
	bool enabled = true;
	bool use_pattern = false;
	int frames = 0;

	CHandle h_transform;

	int  shadows_resolution = 1024;
	float shadows_step = 1.75f;
	float shadows_bias = 0.00001f;

	float intensity = 1.0f;
	float godrays_intensity = 1.0f;
	float irr_intensity = 1.0f;

	static void registerMsgs() {
		DECL_MSG(TCompLightSpot, TMsgAllEntitiesCreated, onAllEntitiesCreated);
	}

	void load(const json& j, TEntityParseContext& ctx);
	void onEntityCreated();
	void onAllEntitiesCreated(const TMsgAllEntitiesCreated& msg);
	void update(float dt);
	void debugInMenu();
	void renderDebug();
	bool activate();
	void generateShadowMap();
	MAT44 getWorld() const;
};