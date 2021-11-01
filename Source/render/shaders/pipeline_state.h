#pragma once

#include "render/render_utils.h"

class CTexture;

class CPipelineState : public IResource {

  CVertexShader vs;
  CPixelShader  ps;
  json          jcfg;
  RSConfig      rs_config = RSConfig::DEFAULT;
  ZConfig       z_config = ZConfig::DEFAULT;
  BlendConfig   blend_config = BlendConfig::DEFAULT;

  const CTexture* env = nullptr;
  const CTexture* env_prefiltered = nullptr;
  const CTexture* brdf_lut = nullptr;
  const CTexture* noise = nullptr;

  static const CPipelineState* curr_pipeline;

  bool use_instanced = false;
  bool use_skin = false;

public:

  void activate() const;
  void destroy();
  bool create(const json& j);

  bool renderInMenu() const;
  const json& getConfig() const { return jcfg; }

  bool useInstanced() const { return use_instanced; }
  bool useSkin() const { return use_skin; }

};
