#include "mcv_platform.h"
#include "comp_color_grading.h"

DECL_OBJ_MANAGER("color_grading", TCompColorGrading);

// Export with the photoshop plugin: (v8.55.0109.1800)
// - Volume Texture
// - No Mip Maps
// - ARGB 32 bpp unsigned

// ---------------------
void TCompColorGrading::debugInMenu() {
  ImGui::Checkbox("Enabled", &enabled);
  ImGui::DragFloat("Amount", &amount, 0.01f, 0.0f, 1.0f);
}

void TCompColorGrading::load(const json& j, TEntityParseContext& ctx) {
  enabled = j.value("enabled", enabled);
  amount = j.value("amount", amount);
  std::string lut_name = j.value("lut", "");
  lut = Resources.get(lut_name)->as<CTexture>();
}

