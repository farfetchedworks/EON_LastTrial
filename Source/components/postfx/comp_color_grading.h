#pragma once

#include "components/common/comp_base.h"

class CTexture;
class CRenderToTexture;

// ------------------------------------
struct TCompColorGrading : public TCompBase
{
  bool                          enabled = true;
  float                         amount = 1.f;
  // lookup table, translates rgb to rgb's
  const CTexture* lut = nullptr;

  void load(const json& j, TEntityParseContext& ctx);
  void debugInMenu();
  float getAmount() const {
    return enabled ? amount : 0.f;
  }

};

