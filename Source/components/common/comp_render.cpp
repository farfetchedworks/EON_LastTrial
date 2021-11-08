#include "mcv_platform.h"
#include "comp_render.h"
#include "comp_transform.h"
#include "render/render.h"
#include "render/textures/material.h"
#include "render/render_manager.h"
#include "render/draw_primitives.h"
#include "render/meshes/mesh_io.h"

DECL_OBJ_MANAGER("render", TCompRender)

//struct TDrawCall {
//  const CMesh* mesh = nullptr;
//  uint32_t         submesh_idx = 0;
//  const CMaterial* material = nullptr;
//};

//std::vector< TDrawCall > draw_calls;

float TCompRender::TDrawCall::render_tangent_space_scale = 0.02f;

void TCompRender::registerMsgs() {
  DECL_MSG(TCompRender, TMsgDefineLocalAABB, onDefineLocalAABB);
}

void TCompRender::onDefineLocalAABB(const TMsgDefineLocalAABB& msg) {
  // someone wants to know my bounding box
  assert(msg.aabb);
  if (msg.aabb->Extents.x == 0 && msg.aabb->Extents.y == 0 && msg.aabb->Extents.z == 0)
    *msg.aabb = aabb;
  else
    AABB::CreateMerged(*msg.aabb, *msg.aabb, aabb);
}

TCompRender::~TCompRender() {
  removeFromRenderManager();
}

void TCompRender::removeFromRenderManager() {
  CHandle h_this(this);
  RenderManager.delKeysFromOwner(h_this);
}

bool TCompRender::TDrawCall::load(const json& j) {
  mesh = Resources.get(j["mesh"])->as<CMesh>();
  mesh_group = j.value("mesh_group", 0);
  material = Resources.get(j["mat"])->as<CMaterial>();
  tag = j.value("tag", tag);
  enabled = j.value("enabled", enabled);
  uint32_t submesh_idx = 0;
  return true;
}

bool TCompRender::TDrawCall::debugInMenu() {
  bool changed = false;
  ImGui::LabelText("Mesh", "%s", mesh ? mesh->getName().c_str() : "null" );
  changed |= ImGui::Checkbox("Enabled", &enabled);
  changed |= material->renderInMenu();
  changed |= ImGui::Checkbox("Show Tangent Space", &show_tangent_space);
  if(show_tangent_space)
    ImGui::DragFloat("Draw Scale", &TDrawCall::render_tangent_space_scale, 0.01f, 0.001f, 1.0f);
  return changed;
}

void TCompRender::TDrawCall::drawTangentSpace(MAT44 world) {

  if (!cpu_mesh) {
    cpu_mesh = new TMeshIO();
    CFileDataProvider fdp(mesh->getName().c_str());
    bool is_ok = cpu_mesh->read(fdp);
    assert(is_ok);
  }

  if (cpu_mesh && strcmp(cpu_mesh->header.vertex_type_name, "PosNUvTan") == 0) {


    const VtxPosNUvT* v = (const VtxPosNUvT*)cpu_mesh->vertices.data();
    for (uint32_t i = 0; i < cpu_mesh->header.num_vertices; ++i, ++v) {
      VEC3 p0 = v->pos;
      VEC3 pz = p0 + v->normal * render_tangent_space_scale;
      VEC3 px = p0 + v->tangent * render_tangent_space_scale;
      VEC3 binormal = v->normal.Cross(v->tangent) * v->tangent_w;
      VEC3 py = p0 + binormal * render_tangent_space_scale;
      
      VEC3 w0 = VEC3::Transform(p0, world);
      VEC3 wx = VEC3::Transform(px, world);
      VEC3 wy = VEC3::Transform(py, world);
      VEC3 wz = VEC3::Transform(pz, world);
      drawLine(w0, wz, Colors::Blue);
      drawLine(w0, wx, Colors::Red);
      drawLine(w0, wy, Colors::Green);
    }
  }
}

void TCompRender::debugInMenu() {
  bool changed = false;
  for (auto& dc : draw_calls) {
    ImGui::PushID(&dc);
    changed |= dc.debugInMenu();
    ImGui::PopID();
  }
  if (changed) 
    updateRenderManager();
}

void TCompRender::renderDebug() {

  TCompTransform* c_transform = get<TCompTransform>();
  MAT44 world = c_transform->asMatrix();

  for (auto& dc : draw_calls) {
    drawPrimitive(dc.mesh, world, Colors::White);
    if (dc.show_tangent_space) {
      TCompTransform* c_transform = get<TCompTransform>();
      dc.drawTangentSpace(c_transform->asMatrix());
    }
  }
}

void TCompRender::setMaterialForAll(const CMaterial* mat) {
    for (auto& dc : draw_calls) {
        dc.material = mat;
    }
    updateRenderManager();
}

void TCompRender::enableMeshesWithTag(int tag) {
    for (auto& dc : draw_calls) 
        dc.enabled = (dc.tag == tag);
    updateRenderManager();
}

void TCompRender::updateRenderManager() {
  
  removeFromRenderManager();

  CHandle h_this(this);

  if (!h_this.isValid())
      return;

  for (auto& dc : draw_calls) {
    if (!dc.enabled)
      continue;
    RenderManager.addKey(
      h_this, 
      dc.mesh,
      dc.mesh_group,
      dc.material
    );
  }
}

void TCompRender::onEntityCreated() {
  updateRenderManager();
}

void TCompRender::setEnabled(bool v)
{
    for (auto& dc : draw_calls) {
        dc.enabled = v;
    }
    updateRenderManager();
}

void TCompRender::updateAABB() {
  for (size_t i = 0; i < draw_calls.size(); ++i) {
    const AABB& dc_aabb = draw_calls[i].mesh->getAABB();
    if (i == 0)
      aabb = dc_aabb;
    else
      AABB::CreateMerged(aabb, dc_aabb, aabb);
  }
}

void TCompRender::load(const json& j, TEntityParseContext& ctx) {
  if (j.is_array()) {

    for (auto& jit : j.items()) {
      const json& jentry = jit.value();
      TDrawCall dc;
      if (dc.load(jentry)) 
        draw_calls.push_back(dc);
    }

    updateAABB();

  }
}
