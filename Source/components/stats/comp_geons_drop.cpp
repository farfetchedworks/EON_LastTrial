#include "mcv_platform.h"
#include "handle/handle.h"
#include "comp_geons_drop.h"
#include "entity/entity.h"

DECL_OBJ_MANAGER("geons_drop", TCompGeonsDrop)

void TCompGeonsDrop::load(const json& j, TEntityParseContext& ctx)
{
  geons_dropped = j.value("geons_dropped", geons_dropped);
}

void TCompGeonsDrop::debugInMenu()
{
  ImGui::DragInt("Geons dropped: ", &geons_dropped, 2.f, 0, 1000);
  ImGui::Separator();
}

