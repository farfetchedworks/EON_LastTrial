#include "mcv_platform.h"
#include "components/common/comp_base.h"
#include "components/common/comp_transform.h"
#include "entity/entity.h"

struct TCompAimTo : public TCompBase {

  std::string target;

  // To be cached...
  CHandle h_target_transform;
  CHandle h_my_transform;

  void load(const json& j, TEntityParseContext& ctx) {
    target = j.value("target", "");
  }

  void debugInMenu() {
    ImGui::Text("%s", target.c_str());
  }

  void update(float dt) {
    // ...

    // target_name->CHandle->CEntity(solo la primera vez)
    CEntity* e_target = getEntityByName(target);
    if (!e_target)
      return;

    // Entity -> Su Transform
    TCompTransform* c_target = e_target->get<TCompTransform>();
    VEC3 target_pos = c_target->getPosition();

    // Access to my owner entity
    CEntity* my_entity = CHandle(this).getOwner();
    TCompTransform* c_my_transform = my_entity->get<TCompTransform>();
    c_my_transform->lookAt(c_my_transform->getPosition(), target_pos, VEC3::Up);

  }

};

DECL_OBJ_MANAGER("aim_to", TCompAimTo)