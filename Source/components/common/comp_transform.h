#pragma once

#include "comp_base.h"

class TCompTransform : public TCompBase, public CTransform {

	DECL_SIBLING_ACCESS();

	bool use_parent_transform = true;

public:

  void load(const json& j, TEntityParseContext& ctx);
  void set(const CTransform& new_tmx);
  void debugInMenu();
  void renderDebug();
  void update(float dt);
};
