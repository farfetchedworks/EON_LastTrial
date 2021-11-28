#include "mcv_platform.h"
#include "comp_culling.h"
#include "comp_aabb.h"
#include "comp_camera.h"
#include "comp_light_spot.h"

DECL_OBJ_MANAGER("culling", TCompCulling);

TCompCulling::TCullingBits* TCompCulling::debug_culling_bits = nullptr;

void TCompCulling::debugInMenu() {
  auto hm = getObjectManager<TCompAbsAABB>();
  ImGui::Text("%d / %d AABB's visible", bits.count(), hm->size());
  if (debug_culling_bits != &bits) {
    if (ImGui::SmallButton("Colorize Culled AABB's"))
      debug_culling_bits = &bits;
  }
}

// Si algun plano tiene la caja en la parte negativa
// completamente, el aabb no esta dentro del set de planos
bool TCompCulling::VPlanes::isVisible(const AABB* aabb) const {
  auto it = begin();
  while (it != end()) {
    if (it->isCulled(aabb))
      return false;
    ++it;
  }
  return true;
}

void TCompCulling::updateFromMatrix(MAT44 view_proj) {
  // Construir el set de planos usando la view_proj
  planes.fromViewProjection(view_proj);

  // Start from zero
  bits.reset();

  // Traverse all aabb's defined in the game
  // and test them
  // Use the AbsAABB index to access the bitset
  auto hm = getObjectManager<TCompAbsAABB>();
  hm->forEachWithExternalIdx([this](const TCompAbsAABB* aabb, uint32_t external_idx) {
    if (planes.isVisible(aabb))
      bits.set(external_idx);
    });
}

void TCompCulling::setHasToUpdate(bool has_to_update)
{
    _has_to_update = has_to_update;
}

void TCompCulling::update(float dt) {
    PROFILE_FUNCTION("Updating culling");

    if (!_has_to_update) return;

    // Or send a msg to the entity to get the view proj
    if (TCompCamera* c_camera = get<TCompCamera>())
        updateFromMatrix(c_camera->getViewProjection());
    else if (TCompLightSpot* c_light_spot = get<TCompLightSpot>())
        updateFromMatrix(c_light_spot->getViewProjection());
}