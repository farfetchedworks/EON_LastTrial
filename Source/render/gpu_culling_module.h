#pragma once

#include "modules/module.h"
#include "components/common/comp_compute.h"
#include "components/common/comp_buffers.h"
#include "render/render_channel.h"

class CMaterial;

class CGPUCullingModule : public IModule
{

  bool                          show_debug = true;
  bool                          is_dirty = false;
  bool                          culling_shadows = false;
  eRenderChannel                my_render_channel;
  eRenderChannel                supported_render_channels[4];
  std::string                   suffix;

  // -----------------------------------------------
  // All objects in the scene, input of the culling
  // Size MUST match the size of the gpu_instances
  // This structure is used in the GPU
  struct TObj {
    VEC3      aabb_center;    // AABB
    uint32_t  prefab_idx;     // In the array of prefabs
    VEC3      aabb_half;
    uint32_t  dummy2 = 0;
    MAT44     world;          // Matrix
  };

  // -----------------------------------------------
  // This struct is also used by the GPU
  struct TPrefab {
    static const uint32_t max_render_types_per_prefab = 6;
    CHandle   id;
    uint32_t  lod_prefab = -1;
    float     lod_threshold = 1e5;
    uint32_t  num_objs = 0;             // Number of instances of this type
    uint32_t  num_render_type_ids = 0;  // How many parts we must add
    uint32_t  total_num_objs = 0;       // Takes into account if another prefab is using me to render a LOD
    uint32_t  render_type_ids[max_render_types_per_prefab] = { 0,0,0,0,0,0 };
  };

  // -----------------------------------------------
  // Each draw call requires this 5 uint's as arguments
  // We can command the GPU to execute a DrawIndexedInstance
  // using the args store in some GPU buffer.
  struct DrawIndexedInstancedArgs {
    uint32_t indexCount = 0;
    uint32_t instanceCount = 0;     // at offset 4
    uint32_t firstIndex = 0;
    uint32_t firstVertex = 0;
    uint32_t firstInstance = 0;
  };

  // -----------------------------------------------
  // Upload to the GPU so it can fill the num_instances
  // The CPU will fill the args, and the base
  // 1 prefab can have several TDrawDatas
  struct TDrawData {
    DrawIndexedInstancedArgs args;      // 5 ints
    uint32_t                 base = 0;  // Where in the array of culled_instances we can start adding objs
    uint32_t                 max_instances = 0;  // How many instances of this render type we will ever use
    uint32_t                 dummy = 0;
  };

  /*
  // You can paste this into RenderDoc to visualize the contents of the raw data
  struct TVisibleObj
  {
      uint indexCount;
      uint instanceCount;
      uint firstIndex;
      uint firstVertex;
      uint firstInstance;
      uint base;
      uint max_instances;
      uint dummy;
  }
  */

  // -----------------------------------------------
  struct TCtes {
    uint32_t total_num_objs = 0;
    uint32_t instance_base = 0;
    uint32_t instancing_padding[2];
  };

  // -----------------------------------------------
  struct TCullingPlanes {
    VEC4  planes[6];
    VEC3  CullingCameraPos;
    float dummy;
  };

  // -----------------------------------------------
  // This are the keys that make the obj type unique
  // This structure is NOT used in the GPU
  struct TRenderTypeID {
    const CMesh* mesh = nullptr;
    uint32_t         subgroup = 0;
    const CMaterial* material = nullptr;
    char             title[64];
    bool operator==(const TRenderTypeID& other) const {
      return mesh == other.mesh && subgroup == other.subgroup && material == other.material;
    }
  };

  // -----------------------------------------------
  std::vector< TObj >           objs;
  std::vector< TPrefab >        prefabs;
  std::vector< TDrawData >      draw_datas;
  std::vector< TRenderTypeID >  render_types;
  uint32_t                      max_objs = 0;
  uint32_t                      max_prefabs = 0;
  uint32_t                      max_draw_datas = 0;

  TCullingPlanes                culling_planes;
  TCtes                         ctes;

  // -----------------------------------------------
  std::string                   entity_camera_name;
  CHandle                       h_camera;

  // -----------------------------------------------
  // Store my compute shaders and compute buffers associated to the module here.
  TCompCompute                  comp_compute;
  TCompBuffers                  comp_buffers;
  CGPUBuffer* gpu_objs = nullptr;
  CGPUBuffer* gpu_draw_datas = nullptr;
  CGPUBuffer* gpu_prefabs = nullptr;
  CBaseShaderCte* gpu_ctes = nullptr;

  void updateCamera();
  void updateCullingPlanes(const CCamera& camera);
  void clearRenderDataInGPU();
  void preparePrefabs();
  void setPrefabLod(uint32_t high_prefab_idx, uint32_t low_prefab_idx, float threshold);
  uint32_t addPrefabInstance(CHandle new_id);
  uint32_t registerPrefab(CHandle new_id);
  uint32_t addRenderType(const TRenderTypeID& new_render_type);

public:

  CGPUCullingModule(const std::string& name, eRenderChannel render_channel) : IModule(name) {
      my_render_channel = render_channel;
      culling_shadows = (my_render_channel == RC_SHADOW_CASTERS);
      suffix = culling_shadows ? "::shadows" : "";
  }

  bool start() override;
  void stop() override;
  void renderDebug();
  void renderInMenu();

  void setCamera(CHandle camera);

  // ---------------------------------------------------------------
  void addToRender(CHandle h_prefab, const AABB aabb, const MAT44 world);
  void run();
  void reset();
  void renderAll(eRenderChannel render_channel);
};
