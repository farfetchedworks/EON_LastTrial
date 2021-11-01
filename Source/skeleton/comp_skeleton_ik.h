#pragma once

#include "components/common/comp_base.h"
#include "skeleton/ik_handler.h"

struct TCompSkeletonIK : public TCompBase {
  float       delta_y_over_c = 0.0f;
  float       amount = 1.0f;
  std::string bone_name;
  int         bone_id_c = -1;
  VEC3        AB_dir = VEC3(0,-1,0);

  TIKHandle ik;

  void load(const json& j, TEntityParseContext& ctx);
  void update(float elapsed);
  void debugInMenu();
  void renderDebug();
};

