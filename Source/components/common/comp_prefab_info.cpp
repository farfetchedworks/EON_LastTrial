#include "mcv_platform.h"
#include "comp_prefab_info.h"

DECL_OBJ_MANAGER("from_prefab", TCompPrefabInfo)

void TCompPrefabInfo::load(const json& j, TEntityParseContext& ctx) {
    assert(j.is_string());
    src = j;
}

void TCompPrefabInfo::debugInMenu() {
  ImGui::Text(src.c_str());
}