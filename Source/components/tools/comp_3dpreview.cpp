#include "mcv_platform.h"
#include "entity/entity.h"
#include "engine.h"
#include "components/common/comp_base.h"
#include "input/input_module.h"
#include "modules/module_camera_mixer.h"

class TComp3DPreview : public TCompBase {

  DECL_SIBLING_ACCESS();

public:

  void load(const json& j, TEntityParseContext& ctx) {
    
  }

  void onEntityCreated() {
      PlayerInput.blockInput();
  }

  void debugInMenu() {
      if (ImGui::Button("Toggle input")) {
          PlayerInput.toggleBlockInput();
      }
  }
};

DECL_OBJ_MANAGER("3dpreview", TComp3DPreview)