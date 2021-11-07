#pragma once

#include "comp_base.h"
#include "components/messages.h"

class CMesh;
class CMaterial;

// Render information associated to a single entity
class TCompRender : public TCompBase {

  void removeFromRenderManager();
  void updateAABB();
  AABB aabb;

public:

  DECL_SIBLING_ACCESS();

  struct TDrawCall {
    const CMesh*     mesh = nullptr;
    uint32_t         mesh_group = 0;
    const CMaterial* material = nullptr;
    bool             enabled = true;
    bool             show_tangent_space = false;
    int              tag = 0;
    // Debug
    TMeshIO*         cpu_mesh = nullptr;
    bool load(const json& j);
    bool debugInMenu();
    void drawTangentSpace(MAT44 world);

    static float render_tangent_space_scale;

  };

  ~TCompRender();
  void debugInMenu();
  void renderDebug();
  void onEntityCreated();
  void setMaterialForAll(const CMaterial* mat);
  void enableMeshesWithTag(int tag);
  void setEnabled(bool v);

  void load(const json& j, TEntityParseContext& ctx);
  void onDefineLocalAABB(const TMsgDefineLocalAABB& msg);

  std::vector< TDrawCall > draw_calls;
  void updateRenderManager();

  static void registerMsgs();
};
