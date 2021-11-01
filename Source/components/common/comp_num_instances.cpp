#include "mcv_platform.h"
#include "comp_num_instances.h"

DECL_OBJ_MANAGER("num_instances", TCompNumInstances)

void TCompNumInstances::debugInMenu() {
  if (is_indirect) {
    ImGui::Text("Draw is indirect from buffer %s", gpu_buffer_name.c_str());
  }
  else {
    ImGui::DragInt("Num Instances", (int*)&num_instances, 0.1f, 0, 2048);
  }

}

void TCompNumInstances::load(const json& j, TEntityParseContext& ctx) {
  if (j.is_string()) {
    gpu_buffer_name = j.get<std::string>();
    is_indirect = true;
    num_instances = 0;
  }
  else
    num_instances = j.get<uint32_t>();
}
