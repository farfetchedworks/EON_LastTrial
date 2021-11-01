#pragma once
#include "comp_base.h"
#include "entity/entity.h"

class TCompPrefabInfo : public TCompBase {

  DECL_SIBLING_ACCESS();
  std::string src;

public:

  void load(const json& j, TEntityParseContext& ctx);
  void debugInMenu();
  const std::string* getPrefabName() const { return &src; }
};
