#pragma once

#include "comp_base.h"
#include "geometry/camera.h"
#include "entity/entity.h"

class TCompCamera : public CCamera, public TCompBase {
  
  DECL_SIBLING_ACCESS();

public:

  void load(const json& j, TEntityParseContext& ctx);
  void update(float dt);
  void debugInMenu();
  bool innerDebugInMenu();
};