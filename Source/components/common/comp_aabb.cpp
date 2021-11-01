#include "mcv_platform.h"
#include "comp_aabb.h"
#include "comp_transform.h"
#include "render/draw_primitives.h"
#include "components/messages.h"
#include "components/common/comp_culling.h"

DECL_OBJ_MANAGER("abs_aabb", TCompAbsAABB);
DECL_OBJ_MANAGER("local_aabb", TCompLocalAABB);

// Model * ( center +/- halfsize ) = model * center + model * half_size
AABB getRotatedBy(AABB src, const MAT44& model) {
  AABB new_aabb;
  new_aabb.Center = VEC3::Transform(src.Center, model);
  new_aabb.Extents.x = src.Extents.x * fabsf(model(0, 0))
    + src.Extents.y * fabsf(model(1, 0))
    + src.Extents.z * fabsf(model(2, 0));
  new_aabb.Extents.y = src.Extents.x * fabsf(model(0, 1))
    + src.Extents.y * fabsf(model(1, 1))
    + src.Extents.z * fabsf(model(2, 1));
  new_aabb.Extents.z = src.Extents.x * fabsf(model(0, 2))
    + src.Extents.y * fabsf(model(1, 2))
    + src.Extents.z * fabsf(model(2, 2));
  return new_aabb;
}

void TCompAABB::load(const json& j, TEntityParseContext& ctx) {
  if (j.is_object()) {
    Center = loadVEC3(j, "center");
    Extents = loadVEC3(j, "half_size");
  }
  else {
    Center = VEC3::Zero;
    Extents = VEC3::Zero;
  }
}

void TCompAABB::debugInMenu() {
  ImGui::DragFloat3("center", &Center.x, 0.01f, -25.0f, 25.0f);
  ImGui::DragFloat3("half_size", &Extents.x, 0.01f, 0.0f, 25.0f);
}

// ----------------------------------------------------------------
void TCompAABB::updateFromSiblingsLocalAABBs(CEntity* e) {
  e->sendMsg(TMsgDefineLocalAABB{ this });
}

// ----------------------------------------------------------------
void TCompAbsAABB::renderDebug() {
#ifdef _DEBUG
  VEC4 color = Colors::Green;
  // Use red if the culling bits are defined are we have been culled
  if (TCompCulling::debug_culling_bits && !TCompCulling::debug_culling_bits->test(CHandle(this).getExternalIndex()))
    color = Colors::Red;
  drawWiredAABB(*this, MAT44::Identity, color);
#endif
}

void TCompAbsAABB::onEntityCreated() {
  TCompTransform* c_transform = get<TCompTransform>();
  // Get the local aabb in 'this'
  updateFromSiblingsLocalAABBs(getEntity());
  // Convert local aabb into abs aabb
  *(AABB*)this = getRotatedBy(*this, c_transform->asMatrix());
}

// ----------------------------------------------------------------
void TCompLocalAABB::onEntityCreated() {
  updateFromSiblingsLocalAABBs(getEntity());
}

void TCompLocalAABB::renderDebug() {
#ifdef _DEBUG
    TCompTransform* c_transform = get<TCompTransform>();
    assert(c_transform);
    drawWiredAABB(*this, c_transform->asMatrix(), Colors::Orange);
#endif
}

void TCompLocalAABB::update( float dt ) {
  TCompTransform* c_transform = get<TCompTransform>();
  TCompAbsAABB* c_abs_aabb = get<TCompAbsAABB>();
  if(c_transform && c_abs_aabb)
    *(AABB*)c_abs_aabb = getRotatedBy(*this, c_transform->asMatrix());
}



