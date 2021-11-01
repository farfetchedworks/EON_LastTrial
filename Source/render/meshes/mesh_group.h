#pragma once

struct TMeshGroup {
  uint32_t first_index = 0;
  uint32_t num_indices = 0;
};
using VMeshGroups = std::vector<TMeshGroup>;
