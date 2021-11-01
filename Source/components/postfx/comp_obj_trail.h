#pragma once
#include "comp_bilateral_blur.h"

class CTexture;
class CRenderToTexture;

struct TCompObjectTrail : public TCompBilateralBlur
{
  int idx = 0;

  bool enabled = true;
  int max_frames = 5;
  int frames = 0;

  CRenderToTexture* current_trails = nullptr;
  CRenderToTexture* rt_trail = nullptr;
  std::function<CTexture* (CTexture*, CTexture*)> apply_fn;
  CShaderCte<CtesObjTrail>  ctes;

  TCompObjectTrail();
  ~TCompObjectTrail();

  void load(const json& j, TEntityParseContext& ctx);
  void onEntityCreated();
  void debugInMenu();
  void enable(bool status);

  CTexture* renderTrailFrames(CTexture* in_texture, CTexture* last_texture);

  // ------------- render in fx stack -------------
  void onRender(const TMsgRenderPostFX& msg);
  static void registerMsgs() {
	  DECL_MSG(TCompObjectTrail, TMsgRenderPostFX, onRender);
  }
};

