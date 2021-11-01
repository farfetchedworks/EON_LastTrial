#pragma once

#include "components/common/comp_base.h"
#include "entity/entity.h"
#include "render/intel/ASSAO.h"

class CRenderToTexture;
class CTexture;

// ------------------------------------
class TCompRenderAO : public TCompBase
{
  CRenderToTexture* rt_output = nullptr;
  ASSAO_Settings    settings;
  ASSAO_InputsDX11  assao_inputs;
  bool              enabled = true;
  int               xres = 0;
  int               yres = 0;
  
  DECL_SIBLING_ACCESS();

  void prepareRenderTarget();

public:

  void  load(const json& j, TEntityParseContext& ctx);
  void  debugInMenu();
  const CTexture* compute(CTexture* normals, ASSAO_Effect* fx);
};
