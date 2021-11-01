#include "mcv_platform.h"
#include "entity/entity.h"
#include "components/common/comp_base.h"
#include "components/messages.h"

class TCompJohnLife : public TCompBase {

  DECL_SIBLING_ACCESS();

  float life = 100.0f;

public:

  static void registerMsgs() {
    DECL_MSG(TCompJohnLife, TMsgExplosion, onExplosion);
  }

  void load(const json& j, TEntityParseContext& ctx) {
    life = j.value("life", life);
  }

  void onExplosion(const TMsgExplosion& msg) {
    float new_life = std::max(life - msg.damage, 0.0f);
    dbg("Recv explosion %f. New life %f", msg.damage, new_life);
    life = new_life;
    // if( life < 0 ) ... killed
  }

  void debugInMenu() {
    ImGui::Text("Life: %f", life);
  }

};

DECL_OBJ_MANAGER("john_life", TCompJohnLife)