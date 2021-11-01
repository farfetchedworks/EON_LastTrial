#ifndef INC_COMP_LOD_H_
#define INC_COMP_LOD_H_

#include "comp_base.h"

// ----------------------------------------
struct TCompLod : public TCompBase {
  std::string replacement_prefab;
  float       threshold = 250000.0f;
  
  void load(const json& j, TEntityParseContext& ctx) {
    threshold = j.value("threshold", threshold);
    replacement_prefab = j.value("replacement_prefab", replacement_prefab);
  }

};

#endif
