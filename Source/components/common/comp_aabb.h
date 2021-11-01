#pragma once

#include "geometry/geometry.h"
#include "components/common/comp_base.h"
#include "entity/entity.h"

struct TCompAABB : public AABB, public TCompBase {
  void load(const json& j, TEntityParseContext& ctx);
  void debugInMenu();
  void updateFromSiblingsLocalAABBs(CEntity* e);
};

struct TCompAbsAABB : public TCompAABB {
  DECL_SIBLING_ACCESS();
  void renderDebug();
  void onEntityCreated();
};

struct TCompLocalAABB : public TCompAABB {
  DECL_SIBLING_ACCESS();
  void renderDebug();
  void update( float dt );
  void onEntityCreated();
};
