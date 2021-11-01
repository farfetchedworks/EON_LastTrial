#pragma once

#include "comp_base.h"
#include "entity/entity.h"

class TCompName : public TCompBase {

  static const size_t max_size = 64;
  char name[max_size];

  DECL_SIBLING_ACCESS();

public:

  // Dictionary to map names to Handles
  static std::unordered_map< std::string, CHandle > all_names;

  // Store prefab information
  static std::unordered_map< std::string, VHandles > all_prefabs;

  const char* getName() const { return name; }
  void setName(const char* new_name);
  void debugInMenu();
  void renderDebug();
  void load(const json& j, TEntityParseContext& ctx);
};
