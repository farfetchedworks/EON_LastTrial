#pragma once

#include "comp_base.h"
#include "comp_camera.h"
#include "render/textures/render_to_texture.h"

class TCompLightPoint : public TCompCamera {
  DECL_SIBLING_ACCESS();
public:
  
  VEC4  color = Colors::White;
  float intensity = 1.0f;
  float irr_intensity = 1.0f;
  float radius = 1.0f;
  bool  enabled = true;
  VEC3  position;

  void load(const json& j, TEntityParseContext& ctx);
  void debugInMenu();
  void renderDebug();
  bool activate();
  MAT44 getWorld() const;
};