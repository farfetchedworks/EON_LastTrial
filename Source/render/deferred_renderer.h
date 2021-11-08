#pragma once

class CRenderToTexture;
class ASSAO_Effect;

class CDeferredRenderer {

  int               xres = 0, yres = 0;

  CRenderToTexture* rt_albedos		= nullptr;
  CRenderToTexture* rt_normals		= nullptr;
  CRenderToTexture* rt_normals_aux	= nullptr;
  CRenderToTexture* rt_albedos_aux	= nullptr;
  CRenderToTexture* rt_depth		= nullptr;
  CRenderToTexture* rt_emissive		= nullptr;
  ASSAO_Effect*     assao_fx		= nullptr;
  CHandle           h_camera;

  bool irradiance_cache = false;

  void renderGBuffer();
  void renderDecals();
  void renderAO();
  void renderWater();
  void renderAmbientPass();
  void renderEmissivePass();
  void renderPointLights();
  void renderSpotLights(bool with_shadows);
  void renderSkyBox();
  void renderAutoEmissives();

public:

  CDeferredRenderer();
  bool create(int xres, int yres, bool irradiance_cache = false);
  void destroy();
  void render(CRenderToTexture* out_rt, CHandle new_h_camera, int array_index = 0);
};
