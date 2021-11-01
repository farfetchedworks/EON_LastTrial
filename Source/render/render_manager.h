#pragma once

#include "render_channel.h"

class CMesh;
class CMaterial;
class CGPUBuffer;

class CRenderManager {

public:
  struct TKey {
    const CMesh*     mesh = nullptr;
    const CMaterial* material = nullptr;
    CHandle          h_render_owner;
    CHandle          h_transform;
    CHandle          h_abs_aabb;
    uint16_t         mesh_group : 15;
    uint16_t         is_instanced : 1;
    /*uint16_t         is_auto_instanced : 1;
    uint16_t         is_deleted : 1;
    uint16_t         auto_instanced_idx : 15;
    uint16_t         cast_shadows : 1;*/
    bool renderInMenu();
  };

  /*struct TAutoInstanceData
  {
    int num_instances = 0;
    CGPUBuffer* gpu_buffer;
    VHandles h_render_owners;
    std::vector< MAT44 > transforms;
    bool hasShadows = false;
  };

  std::map<std::string, uint32_t> auto_instances;
  std::vector<TAutoInstanceData> auto_instance_data;*/

protected:
  struct VKeys : std::vector< TKey > {
    void renderInMenu();
  };
  VKeys               keys;
  bool                keys_are_dirty = false;

  void sortKeys();

  std::vector< uint32_t > draw_calls_per_channel;
  std::vector< uint32_t > total_draw_calls_per_channel;

public:

  CRenderManager();

  void addKey(
    CHandle h_render_owner,
    const CMesh* mesh,
    uint32_t submesh_idx,
    const CMaterial* material
    );
  void delKeysFromOwner(CHandle h_render_owner);

  void renderInMenu();
  
  void renderAll(eRenderChannel render_channel, CHandle h_entity_camera);

  void setDirty() {
    keys_are_dirty = true;
  }

};

extern CRenderManager RenderManager;