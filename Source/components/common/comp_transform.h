#pragma once

#include "comp_base.h"

class TCompTransform : public TCompBase, public CTransform {

	bool use_parent_transform = true;

public:

  void load(const json& j, TEntityParseContext& ctx);
  void set(const CTransform& new_tmx);
  void debugInMenu();
  void renderDebug();
};
