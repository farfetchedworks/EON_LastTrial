#pragma once

#include "comp_base.h"

class CGPUBuffer;

struct TCompNumInstances : public TCompBase {
  uint32_t num_instances = 0;
  bool     is_indirect = false;
  CGPUBuffer* gpu_buffer = nullptr;
  std::string gpu_buffer_name;
  void debugInMenu();
  void load(const json& j, TEntityParseContext& ctx);
};