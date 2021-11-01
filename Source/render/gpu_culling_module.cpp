#include "mcv_platform.h"
#include "gpu_culling_module.h"
#include "entity/entity.h"
#include "entity/entity_parser.h"
#include "render/compute/gpu_buffer.h"
#include "render/textures/material.h"
#include "render/draw_primitives.h"
#include "render/shaders/shader_ctes_manager.h"
#include "components/common/comp_transform.h"
#include "components/common/comp_render.h"
#include "components/common/comp_camera.h"
#include "components/common/comp_light_spot.h"
#include "components/common/comp_lod.h"
#include "components/common/comp_aabb.h"

// Same as in material.cpp (keep only one?)
static NamedValues<eRenderChannel>::TEntry culling_render_channel_entries[] = {
  {eRenderChannel::RC_SOLID, "solid"},
  {eRenderChannel::RC_SOLID_STENCIL, "solid_stencil"},
  {eRenderChannel::RC_SHADOW_CASTERS, "shadows"},
  {eRenderChannel::RC_SHADOW_CASTERS_DYNAMIC, "dynamic_shadows"},
  {eRenderChannel::RC_DISTORSIONS, "distorsion"},
  {eRenderChannel::RC_WATER, "water"},
  {eRenderChannel::RC_EMISSIVE, "emissive"},
  {eRenderChannel::RC_TRANSPARENT, "transparent"},
  {eRenderChannel::RC_DECALS, "decals"},
  {eRenderChannel::RC_UI, "ui"},
  {eRenderChannel::RC_DEBUG, "debug"},
};
NamedValues<eRenderChannel> culling_render_channel_names(culling_render_channel_entries, sizeof(culling_render_channel_entries) / sizeof(NamedValues<eRenderChannel>::TEntry));
// -----------------------------------

// defined in comp_aabb.cpp
AABB getRotatedBy(AABB src, const MAT44& model);

// -------------------------------------------------------
// Sample data taken from: https://developer.nvidia.com/orca/speedtree
// This is just to have some fake data to play with
struct TSampleDataGenerator {
  CGPUCullingModule* mod = nullptr;

  VEC3         pmin;
  VEC3         pmax;
  float        scale = 1.0f;
  uint32_t     num_instances = 0;

  VHandles     prefabs;

  void create(const json& j) {

    float radius = j.value("radius", 30.f);
    pmin = VEC3(-radius, 0.f, -radius);
    pmax = VEC3(radius, 1.0f, radius);
    num_instances = j.value("num_instances", num_instances);

    // Read some prefabs and instance them into the world
    std::vector< std::string > prefab_names = j["prefabs"].get< std::vector< std::string > >();
    for (auto& prefab_name : prefab_names) {
      TEntityParseContext ctx;
      bool is_ok = parseScene(prefab_name, ctx);
      assert(is_ok);
      prefabs.push_back(ctx.entities_loaded[0]);
    }

    generate();
  }

  void generate() {

    assert(mod);
    assert(!prefabs.empty());

    for (uint32_t i = 0; i < num_instances; ++i) {

      // Choose a random prefab from the list
      int idx = rand() % prefabs.size();

      // Access the prefab recently created
      CHandle prefab = prefabs[idx];
      CEntity* e = prefab;
      assert(e);

      // Place in a random position
      VEC3 center = pmin + (pmax - pmin) * VEC3(Random::unit(), Random::unit(), Random::unit());
      float sc = scale;

      // Take original transform of the prefab (may contain rotation & scale already)
      TCompTransform* c_transform = e->get< TCompTransform >();

      // Add a random yaw
      MAT44 rot_yaw = MAT44::CreateFromAxisAngle(VEC3(0, 1, 0), Random::unit() * deg2rad(360.0f));

      MAT44 world = c_transform->asMatrix() * rot_yaw * MAT44::CreateScale(sc) * MAT44::CreateTranslation(center);

      // Find AABB in world space
      //TCompRender* cr = e->get< TCompRender >();
      TCompLocalAABB* aabb_local = e->get<TCompLocalAABB>();
      //const TCompRender::TDrawCall& dc = cr->draw_calls[1];       // Use group 1 -> bark
      //AABB aabb_local = dc.mesh->getAABB();
      AABB aabb_abs = getRotatedBy(*aabb_local, world);

      // Register object & matrix to be rendered
      mod->addToRender(prefab, aabb_abs, world);
    }

    // Delete the prefabs we have been using to create the fake data
    for (auto h_prefab : prefabs)
      h_prefab.destroy();
    prefabs.clear();
  }
};

// -------------------------------------------------------
bool CGPUCullingModule::start() {

  json j = loadJson(culling_shadows ? "data/gpu_culling_shadows.json" : "data/gpu_culling.json");

  if (!ShaderCtesManager.isDefined("TCtes")) {
      ShaderCtesManager.registerStruct("TCtes", CShaderCtesManager::makeBasicFactory<TCtes>(CB_SLOT_INSTANCING));
  }

  if (!ShaderCtesManager.isDefined("TCullingPlanes")) {
      ShaderCtesManager.registerStruct("TCullingPlanes", CShaderCtesManager::makeBasicFactory<TCullingPlanes>());
  }
  
  assert(j.count("supported_render_channels"));
  assert(j["supported_render_channels"].is_array());
  assert(j["supported_render_channels"].size() <= 4);
  unsigned int k = 0;
  for (auto& jItem : j["supported_render_channels"]) {
    std::string render_channel_txt = jItem;
    supported_render_channels[k++] = culling_render_channel_names.valueOf(render_channel_txt.c_str());
  }

  show_debug = j.value("show_debug", show_debug);

  // Read a bunch of compute shaders and gpu buffers
  TEntityParseContext ctx;
  comp_compute.load(j["compute"], ctx);
  comp_buffers.load(j["buffers"], ctx);

  // Access buffer to hold the objs in the gpu
  gpu_objs = comp_buffers.getBufferByName("objs");
  assert(gpu_objs || fatal("Missing required buffer to hold the instances to be culled\n"));
  max_objs = gpu_objs->num_elems;
  assert(gpu_objs->bytes_per_elem == sizeof(TObj) || fatal("GPU/CPU struct size don't match for objs %d vs %d\n", gpu_objs->bytes_per_elem, (uint32_t)sizeof(TObj)));

  auto gpu_visible_objs = comp_buffers.getBufferByName("visible_objs");
  assert(gpu_visible_objs);
  assert(gpu_visible_objs->bytes_per_elem == sizeof(MAT44) || fatal("GPU/CPU struct size don't match for visible_objs %d vs %d\n", gpu_visible_objs->bytes_per_elem, (uint32_t)sizeof(MAT44)));

  assert(comp_buffers.getCteByName("TCullingPlanes")->size() == sizeof(TCullingPlanes));

  gpu_ctes = comp_buffers.getCteByName("TCtes");
  assert(gpu_ctes);
  assert(gpu_ctes->size() == sizeof(TCtes));

  gpu_prefabs = comp_buffers.getBufferByName("prefabs");
  max_prefabs = gpu_prefabs->num_elems;
  assert(gpu_prefabs->bytes_per_elem == sizeof(TPrefab) || fatal("GPU/CPU struct size don't match for prefabs %d vs %d\n", gpu_prefabs->bytes_per_elem, (uint32_t)sizeof(TPrefab)));

  gpu_draw_datas = comp_buffers.getBufferByName("draw_datas");
  max_draw_datas = gpu_draw_datas->num_elems;
  assert(gpu_draw_datas->bytes_per_elem == sizeof(TDrawData) || fatal("GPU/CPU struct size don't match for draw_datas %d vs %d\n", gpu_draw_datas->bytes_per_elem, (uint32_t)sizeof(TDrawData)));

  entity_camera_name = j["camera"];
  assert(!entity_camera_name.empty());

  // Reserve in CPU all the memory that we might use, so when we upload cpu data to gpu, we read from valid memory
  // as we upload/read the full buffer size.
  objs.reserve(max_objs);
  prefabs.reserve(max_prefabs);
  render_types.reserve(max_draw_datas);
  draw_datas.reserve(max_draw_datas);

  // populate with some random generated data
  if (j.count("generate_sample_data")) {
    TSampleDataGenerator sample_data;
    sample_data.mod = this;
    sample_data.create(j["generate_sample_data"]);
  }

  return true;
}

void CGPUCullingModule::stop() {

    auto ctes = comp_buffers.getCteByName("TCtes");
    SAFE_DESTROY(ctes);

    ctes = comp_buffers.getCteByName("TCullingPlanes");
    SAFE_DESTROY(ctes);

    auto gpu_visible_objs = comp_buffers.getBufferByName("visible_objs");
    SAFE_DESTROY(gpu_visible_objs);

    SAFE_DESTROY(gpu_objs);
    SAFE_DESTROY(gpu_draw_datas);
    SAFE_DESTROY(gpu_prefabs);
    SAFE_DESTROY(gpu_ctes);
}

// ---------------------------------------------------------------
void CGPUCullingModule::updateCamera() {
  PROFILE_FUNCTION("updateCamera");
  if (!h_camera.isValid()) {
    h_camera = getEntityByName(entity_camera_name);
    if (!h_camera.isValid())
      return;
  }

  float aspect_ratio = (float)Render.getWidth() / (float)Render.getHeight();
  CEntity* e_camera = h_camera;
  CCamera  culling_camera;
  TCompCamera* c_camera = e_camera->get<TCompCamera>();
  if (c_camera) {
    culling_camera = *(CCamera*)c_camera;
  } else {
    TCompLightSpot* c_light_spot = e_camera->get<TCompLightSpot>();
    assert(c_light_spot);
    culling_camera = *(CCamera*)c_light_spot;
    aspect_ratio = c_light_spot->getAspectRatio();
  }

  culling_camera.setProjectionParams(culling_camera.getFov(), aspect_ratio, culling_camera.getNear(), culling_camera.getFar());
  updateCullingPlanes(culling_camera);
}

void CGPUCullingModule::setCamera(CHandle camera)
{
    h_camera = camera;
}

// ---------------------------------------------------------------
void CGPUCullingModule::updateCullingPlanes(const CCamera& camera) {
  MAT44 m = camera.getViewProjection().Transpose();
  VEC4 mx(m._11, m._12, m._13, m._14);
  VEC4 my(m._21, m._22, m._23, m._24);
  VEC4 mz(m._31, m._32, m._33, m._34);
  VEC4 mw(m._41, m._42, m._43, m._44);
  culling_planes.planes[0] = (mw + mx);
  culling_planes.planes[1] = (mw - mx);
  culling_planes.planes[2] = (mw + my);
  culling_planes.planes[3] = (mw - my);
  culling_planes.planes[4] = (mw + mz);      // + mz if frustum is 0..1
  culling_planes.planes[5] = (mw - mz);
  culling_planes.CullingCameraPos = camera.getEye();
}

void CGPUCullingModule::renderDebug() {
  PROFILE_FUNCTION("GPUCulling");
  if (show_debug) {
    for (auto& obj : objs) {
      AABB aabb;
      aabb.Center = obj.aabb_center;
      aabb.Extents = obj.aabb_half;
      drawWiredAABB(aabb, MAT44::Identity, VEC4(1, 0, 0, 1));
    }
  }
}

// ---------------------------------------------------------------
void CGPUCullingModule::addToRender(
  CHandle h_prefab
  , const AABB aabb
  , const MAT44 world
) {

  assert((objs.size() + 1 < max_objs) || fatal("Too many (%d) instances registered. The GPU Buffers 'gpu_objs' need to be larger.\n", max_objs));

  TObj obj;
  obj.aabb_center = aabb.Center;
  obj.prefab_idx = addPrefabInstance(h_prefab);
  obj.aabb_half = aabb.Extents;
  obj.world = world;
  objs.push_back(obj);

  is_dirty = true;
}

uint32_t CGPUCullingModule::addPrefabInstance(CHandle new_id) {
  // Check it the prefab is already registered in our system
  uint32_t idx = 0;
  for (auto& prefab : prefabs) {
    if (prefab.id == new_id) {
      prefab.num_objs++;
      return idx;
    }
    ++idx;
  }
  idx = registerPrefab(new_id);
  prefabs[idx].num_objs++;
  return idx;
}

// ---------------------------------------------------------------
uint32_t CGPUCullingModule::registerPrefab(CHandle new_id) {

  // Register 
  TPrefab prefab;
  prefab.id = new_id;
  prefab.num_objs = 0;
  prefab.num_render_type_ids = 0;

  // Check how many draw calls (instance_types) we have
  CEntity* e = new_id;
  assert(e);
  TCompRender* cr = e->get<TCompRender>();
  assert(cr);
  for (auto& p : cr->draw_calls) {

    // This is the identifier of a draw call
    TRenderTypeID tid;
    tid.subgroup = p.mesh_group;
    tid.mesh = p.mesh;
    tid.material = p.material->getInstancedMaterial();

    if (culling_shadows) {

        const CMaterial* shadows_material = tid.material->getShadowsMaterial();

        if (shadows_material) {
            tid.material = shadows_material;
        }
    }

    // Find which type of instance type is this draw call
    uint32_t render_type_id = addRenderType(tid);

    // Save it
    assert(prefab.num_render_type_ids < TPrefab::max_render_types_per_prefab);
    prefab.render_type_ids[prefab.num_render_type_ids] = render_type_id;
    ++prefab.num_render_type_ids;
  }

  assert(prefabs.size() + 1 < max_prefabs || fatal("We need more space in the gpu buffer 'prefabs'. Current size is %d\n", max_prefabs));
  prefabs.push_back(prefab);
  uint32_t idx = (uint32_t)prefabs.size() - 1;

  // Register a lod if exists a complod in the hi-quality prefab
  TCompLod* c_lod = e->get<TCompLod>();
  if (c_lod) {

    // Load the low-quality prefab
    if (c_lod->replacement_prefab.empty()) {
      setPrefabLod(idx, -1, c_lod->threshold);

    } else {

      TEntityParseContext ctx;
      bool is_ok = parseScene(c_lod->replacement_prefab, ctx);
      CHandle h_lod = ctx.entities_loaded[0];

      uint32_t lod_idx = registerPrefab(h_lod);
      setPrefabLod(idx, lod_idx, c_lod->threshold);

      h_lod.destroy();
    }
  }

  return idx;
}

// ---------------------------------------------------------------
void CGPUCullingModule::setPrefabLod(uint32_t high_prefab_idx, uint32_t low_prefab_idx, float threshold) {
  assert(high_prefab_idx < prefabs.size());
  TPrefab& hq = prefabs[high_prefab_idx];
  hq.lod_prefab = low_prefab_idx;
  hq.lod_threshold = threshold;
}

// ---------------------------------------------------------------
uint32_t CGPUCullingModule::addRenderType(const TRenderTypeID& new_render_type) {

  // Check if exists...
  uint32_t idx = 0;
  for (auto& render_type : render_types) {
    if (render_type == new_render_type)
      return idx;
    ++idx;
  }

  assert((render_types.size() + 1 < max_draw_datas) || fatal("Too many (%d) render_types registered. The GPU Buffer 'draw_datas' need to be larger.\n", max_draw_datas));

  // Register. Copy the key
  render_types.push_back(new_render_type);

  // Create a new name for the prefab
  std::string mesh_name = new_render_type.mesh->getName();
  auto off = mesh_name.find_last_of("/");
  mesh_name = mesh_name.substr(off);
  std::string mat_name = new_render_type.material->getName();
  off = mat_name.find_last_of("/");
  mat_name = mat_name.substr(off);
  snprintf(render_types.back().title, sizeof(TRenderTypeID::title), "%s:%d %s", mesh_name.c_str(), new_render_type.subgroup, mat_name.c_str());

  // Collect the range of triangles we need to render
  TDrawData draw_data = {};
  const TMeshGroup& g = new_render_type.mesh->getGroups()[new_render_type.subgroup];
  draw_data.args.indexCount = g.num_indices;
  draw_data.args.firstIndex = g.first_index;
  draw_datas.push_back(draw_data);

  return idx;
}


// ---------------------------------------------------------------
// This is called when we run the Compute Shaders
void CGPUCullingModule::clearRenderDataInGPU() {
  PROFILE_FUNCTION("clearRenderDataInGPU");

  {
    PROFILE_FUNCTION("Array");
    for (auto& dd : draw_datas)
      dd.args.instanceCount = 0;
  }

  // This line has to be done at least once
  if (!draw_datas.empty()) {
    PROFILE_FUNCTION("copyGPU");
    gpu_draw_datas->copyCPUtoGPUFrom(draw_datas.data());
  }
}

// ---------------------------------------------------------------
void CGPUCullingModule::preparePrefabs() {

  // Count my official number of objects
  for (auto& p : prefabs)
    p.total_num_objs = p.num_objs;

  // Clear counts
  for (auto& dd : draw_datas)
    dd.max_instances = 0;

  for (auto& p : prefabs) {
    // Each prefab will render potencially in several render types
    for (uint32_t idx = 0; idx < p.num_render_type_ids; ++idx) {
      uint32_t render_type_id = p.render_type_ids[idx];
      draw_datas[render_type_id].max_instances += p.total_num_objs;
    }
  }

  // Set the base now that we now how many instances of each render type we have.
  uint32_t base = 0;
  for (auto& dd : draw_datas) {
    dd.base = base;
    base += dd.max_instances;
  }

  uint32_t max_visible_instances = comp_buffers.getBufferByName("visible_objs")->num_elems;
  assert((base <= max_visible_instances) || fatal("We require more space in the buffer %d. Current %d", base, max_visible_instances));

  assert(gpu_prefabs);
  gpu_prefabs->copyCPUtoGPUFrom(prefabs.data());
}


// ---------------------------------------------------------------
// This is called when we run the Compute Shaders
void CGPUCullingModule::run() {

  CGpuScope gpu_scope((std::string("GPU Culling") + suffix).c_str());
  PROFILE_FUNCTION("CGPUCullingModule" + suffix);

  // Just before starting the cs tasks, get the current data of the camera
  updateCamera();

  // Upload culling planes to GPU
  comp_buffers.getCteByName("TCullingPlanes")->updateFromCPU(&culling_planes);

  if (is_dirty) {
    gpu_objs->copyCPUtoGPUFrom(objs.data());

    preparePrefabs();

    // Notify total number of objects we must try to cull
    ctes.total_num_objs = (uint32_t)objs.size();
    gpu_ctes->updateFromCPU(&ctes);

    is_dirty = false;
  }

  clearRenderDataInGPU();

  // Run the culling in the GPU
  comp_compute.executions[0].sizes[0] = (uint32_t)objs.size();
  comp_compute.executions[0].run(&comp_buffers);
}


void CGPUCullingModule::renderInMenu() {

  if (ImGui::TreeNode((std::string("GPU Culling") + suffix).c_str())) {
    ImGui::Checkbox("Show Debug", &show_debug);

    if (ImGui::TreeNode("All objs...")) {
      for (auto& obj : objs)
        ImGui::Text("Prefab:%d at %f %f %f", obj.prefab_idx, obj.aabb_center.x, obj.aabb_center.y, obj.aabb_center.z);
      ImGui::TreePop();
    }

    if (ImGui::TreeNode("Prefabs")) {
      int idx = 0;
      for (auto& p : prefabs) {
        char txt[256];
        sprintf(txt, "[%2d] %3d num_objs with %d render types Lod:%d at %f Total:%d", idx, p.num_objs, p.num_render_type_ids, p.lod_prefab, p.lod_threshold, p.total_num_objs);
        if (ImGui::TreeNode(txt)) {
          if (ImGui::TreeNode("Render Types")) {
            for (uint32_t i = 0; i < p.num_render_type_ids; ++i)
              ImGui::Text("[%d] %d", i, p.render_type_ids[i]);
            ImGui::TreePop();
          }
          if (ImGui::TreeNode("Objs...")) {
            for (auto& obj : objs) {
              if (obj.prefab_idx == idx)
                ImGui::Text("Prefab:%d at %f %f %f", obj.prefab_idx, obj.aabb_center.x, obj.aabb_center.y, obj.aabb_center.z);
            }
            ImGui::TreePop();
          }
          ImGui::TreePop();
        }
        ++idx;
      }
      ImGui::TreePop();
    }

    if (ImGui::TreeNode("Draw Datas")) {
      for (auto& dd : draw_datas)
        ImGui::Text("Base:%3d indices:%4d from:%d", dd.base, dd.args.indexCount, dd.args.firstIndex);
      ImGui::TreePop();
    }

    if (ImGui::TreeNode("GPU Draw Datas")) {
      gpu_draw_datas->copyGPUtoCPU();
      TDrawData* dd = (TDrawData*)gpu_draw_datas->cpu_data.data();
      for (uint32_t i = 0; i < draw_datas.size(); ++i, ++dd) {
        ImGui::Text("Base:%3d Draw Instances:%3d #Idxs:%4d From:%d max_instances:%d dummy:%d", dd->base, dd->args.instanceCount, dd->args.indexCount, dd->args.firstIndex, dd->max_instances, dd->dummy);
      }
      ImGui::TreePop();
    }

    if (ImGui::TreeNode("Render Types")) {
      int idx = 0;
      for (auto& rt : render_types) {
        ImGui::Text("[%d] %d M:%s Mat:%s", idx, rt.subgroup, rt.mesh->getName().c_str(), rt.material->getName().c_str());
        ++idx;
      }
      ImGui::TreePop();
    }

    if (ImGui::TreeNode("Buffers..."))
    {
        comp_buffers.debugInMenu();
        ImGui::TreePop();
    }
    ImGui::TreePop();
  }
}

// ---------------------------------------------------------------
// This is called from the RenderManager
void CGPUCullingModule::renderAll(eRenderChannel render_channel) {

  CGpuScope gpu_scope((std::string("GPU Culling:RenderAll") + suffix).c_str());
  PROFILE_FUNCTION("CGPUCullingModule:RenderAll" + suffix);

  bool is_ok = false;

  for (auto& rc : supported_render_channels) {
      if (rc == render_channel) {
          is_ok = true;
          break;
      }
  }

  if (!is_ok)
      return;

  // Activate in the vs
  gpu_ctes->activate();

  // Offset to the args of the draw indexed instanced args in the draw_datas gpu buffer
  uint32_t offset = 0;
  uint32_t idx = 0;
  for (auto& render_type : render_types) {

    if (render_type.material->getRenderChannel() != render_channel) {
      // Apply offset so we can move to the next instance
      offset += sizeof(TDrawData);
      ++idx;
      continue;
    }

    CGpuScope gpu_render_type(render_type.title);
    PROFILE_FUNCTION(render_type.title);

    // Because SV_InstanceID always start at zero, but the matrices
    // of each group have different starting offset
    ctes.instance_base = draw_datas[idx].base;
    {
      PROFILE_FUNCTION("Cte");
      gpu_ctes->updateFromCPU(&ctes);
    }

    // Setup material & meshes
    {
      PROFILE_FUNCTION("Material");
      render_type.material->activate();
    }

    // Need to get remove the hardcoded value instances & the 0
    const CGPUBuffer* gpu_buffer = comp_buffers.getBufferByName("visible_objs");
    assert(gpu_buffer);
    gpu_buffer->activate(0);

    {
      PROFILE_FUNCTION("Mesh");
      render_type.mesh->activate();
    }

    // Render using as parameters the contents of the buffer at offset 'offset'
    Render.ctx->DrawIndexedInstancedIndirect(gpu_draw_datas->buffer, offset);

    // The offset is in bytes
    offset += sizeof(TDrawData);
    ++idx;
  }

  // Let leave the instance_base = 0 in the GPU so other instancing
  // work
  ctes.instance_base = 0;
  gpu_ctes->updateFromCPU(&ctes);
}
