#include "mcv_platform.h"
#include "render_manager.h"
#include "render/render.h"
#include "render/draw_primitives.h"
#include "render/gpu_culling_module.h"
#include "components/common/comp_transform.h"
#include "components/common/comp_buffers.h"
#include "components/common/comp_num_instances.h"
#include "components/common/comp_culling.h"
#include "components/common/comp_aabb.h"
#include "render/compute/gpu_buffer.h"
#include "skeleton/comp_skeleton.h"
#include "entity/entity.h"
#include "engine.h"

extern CShaderCte<CtesObject> cte_object;
CRenderManager RenderManager;

CRenderManager::CRenderManager()
{
  draw_calls_per_channel.resize(eRenderChannel::RC_COUNT);
  total_draw_calls_per_channel.resize(eRenderChannel::RC_COUNT);
}

void CRenderManager::addKey(
  CHandle h_render_owner,
  const CMesh* mesh,
  uint32_t mesh_group,
  const CMaterial* material
) {

  assert(h_render_owner.isValid());
  assert(mesh);
  assert(material);

  // Recover the handle to the transform associated to this call
  CEntity* entity = h_render_owner.getOwner();

  // entity could be in destroy pending array
  if (!entity)
  {
      dbg("Entity not found (about to be destroyed)\n");
      return;
  }

  TKey key;
  key.material = material;
  key.mesh = mesh;
  key.mesh_group = mesh_group;
  key.h_render_owner = h_render_owner;
  key.h_abs_aabb = CHandle();
  key.h_transform = CHandle(); // Will be completed later

  // If the owner has the comp 'num_instances' your render are instanced.
  TCompNumInstances* c_num_instances = entity->get<TCompNumInstances>();
  key.is_instanced = (c_num_instances != nullptr);

  if (key.is_instanced) {
    assert(key.material->getInstancedMaterial() || fatal( "Mesh %s requires instanced material %s to have instanced support\n", mesh->getName().c_str(), material->getName().c_str()));
    key.material = key.material->getInstancedMaterial();
  }

  if( key.material->usesSkin())
    key.material = key.material->getSkinMaterial();

  keys.push_back(key);

  setDirty();

  // Do we need to register the mesh as a shadow caster...???
  // Do this object casts shadows?
  const CMaterial* shadows_material = key.material->getShadowsMaterial();
  if (shadows_material) {
    key.material = shadows_material;
    keys.push_back(key);
  }
}

void CRenderManager::delKeysFromOwner(CHandle h_render_owner) {
  // Remove will only sort the objects
  auto it = std::remove_if(
    keys.begin(),
    keys.end(),
    [h_render_owner](const TKey& k1) {
      return k1.h_render_owner == h_render_owner;
    }
  );
  // This actually removes the items from the container
  keys.erase(it, keys.end());
}

bool operator<(const CRenderManager::TKey& k, eRenderChannel render_channel) {
  return k.material->getRenderChannel() < render_channel;
}

bool operator<(eRenderChannel render_channel, const CRenderManager::TKey& k) {
  return render_channel < k.material->getRenderChannel();
}

void CRenderManager::sortKeys() {
  PROFILE_FUNCTION("sortKeys");

  std::sort(
    keys.begin(),
    keys.end(),
    [](const TKey& k1, const TKey& k2) {
      if (k1.material != k2.material) {
        if (k1.material->getRenderChannel() != k2.material->getRenderChannel())
          return k1.material->getRenderChannel() < k2.material->getRenderChannel();

        // Both material are different but share the channel
        if (k1.material->getPriority() != k2.material->getPriority())
          return k1.material->getPriority() < k2.material->getPriority();

        return k1.material < k2.material;
      }
      return k1.mesh < k2.mesh;
    }
  );

  for (auto it = keys.begin(); it != keys.end(); ++it)
  {
      TKey& key = *it;

      // get aabb for culling
      if (!key.h_transform.isValid())
      {
          CEntity* entity_owner = key.h_render_owner.getOwner();
          assert(entity_owner);
          key.h_transform = entity_owner->get<TCompTransform>();
          key.h_abs_aabb = entity_owner->get<TCompAbsAABB>();
      }
  }
}

void CRenderManager::renderAll(eRenderChannel render_channel, CHandle h_entity_camera) {
  PROFILE_FUNCTION(render_channel_names.nameOf(render_channel));
  {
    CGpuScope gpu_scope(render_channel_names.nameOf(render_channel));
    {
      PROFILE_FUNCTION(render_channel_names.nameOf(render_channel));
      if (keys_are_dirty) {
        sortKeys();
        keys_are_dirty = false;
      }

      // Retrieve active camera
      CEntity* e_camera = h_entity_camera;
      
      if (!e_camera)
          return;
      
      const TCompCulling* c_culling = e_camera->get<TCompCulling>();
      const TCompCulling::TCullingBits* culling_bits = c_culling ? &c_culling->bits : nullptr;

      // From all the keys, I want just the range where the render_channel maches
      auto range = std::equal_range(
        keys.begin(),
        keys.end(),
        render_channel);
      
      auto it = range.first;
      auto end = range.second;

      uint32_t num_draw_calls = 0;
      draw_calls_per_channel[render_channel] = 0;
      total_draw_calls_per_channel[render_channel] = (uint32_t)std::distance(it, end);

      // PROFILE_FUNCTION("Keys");

      const CMaterial* last_material = nullptr;

      while (it != end) {
        const TKey& key = *it++;

        if (culling_bits) {
          if (key.h_abs_aabb.isValid()) {
            const uint32_t idx = key.h_abs_aabb.getExternalIndex();
            if (!culling_bits->test(idx))
              continue;
          }
        }

        // Unoptimized loop
      
        CEntity* entity = key.h_render_owner.getOwner();
        assert(entity);

        TCompTransform* c_transform = key.h_transform;
        assert(c_transform);

        activateObject(c_transform->asMatrix(), Colors::White, entity);

        if (last_material != key.material) {
            key.material->activate();
            last_material = key.material;
        }

        // if key.material uses a pipeline that requires skinning
        if (key.material->usesSkin())
        {
          // activate the skin cte buffer associated to this draw call.
          TCompSkeleton* c_skel = entity->get<TCompSkeleton>();
          assert(c_skel);
          c_skel->cb_bones.activate();
        }

        key.mesh->activate();

        if (key.is_instanced)
        {
          CEntity* entity = key.h_render_owner.getOwner();

          TCompNumInstances* c_num_instances = entity->get<TCompNumInstances>();
          assert(c_num_instances);

          // activate the skin cte buffer associated to this draw call.
          TCompBuffers* c_buffers = entity->get<TCompBuffers>();
          assert(c_buffers);

          key.material->activateBuffers( c_buffers );

          // The Num Instances is stored in the GPU?
          if (c_num_instances->is_indirect) {
            // The arguments come from a buffer
            if (!c_num_instances->gpu_buffer)
              c_num_instances->gpu_buffer = c_buffers->getBufferByName(c_num_instances->gpu_buffer_name.c_str());
            assert(c_num_instances->gpu_buffer);
            assert(c_num_instances->gpu_buffer->buffer);
            // Read the arguments from a GPU buffer at offset 0
            Render.ctx->DrawIndexedInstancedIndirect(c_num_instances->gpu_buffer->buffer, 0);
          }
          else {
            key.mesh->renderGroupInstanced(key.mesh_group, c_num_instances->num_instances);
          }
        }
        else
        {
          key.mesh->renderGroup(key.mesh_group);
        }

        ++num_draw_calls;
      }

      draw_calls_per_channel[render_channel] = num_draw_calls;
    }
  }

  EngineCulling.renderAll(render_channel);
  EngineCullingShadows.renderAll(render_channel);
}

bool CRenderManager::TKey::renderInMenu() {

  char key_name[256];
  std::string mat_name = material->getName();
  snprintf(key_name, 255, "%s %s [%d]",
    mat_name.c_str(),
    mesh->getName().c_str(),
    mesh_group
  );

  if (ImGui::TreeNode(key_name)) {
    ImGui::SameLine();
    if (ImGui::SmallButton("X")) {
      ImGui::TreePop();
      return false;
    }

    auto ncmaterial = const_cast<CMaterial*>(material);
    if (ImGui::TreeNode(material->getName().c_str())) {
      Resources.edit(&material);
      ncmaterial->renderInMenu();
      ImGui::TreePop();
    }
   
    auto ncmesh = const_cast<CMesh*>(mesh);
    ncmesh->renderInMenu();

    ImGui::TreePop();
  }
  return true;
}

void CRenderManager::VKeys::renderInMenu() {
  for (size_t i = 0; i < size(); ++i) {
    ImGui::PushID((int)i);
    if (!(*this)[i].renderInMenu())
      erase(begin() + i);
    ImGui::PopID();
  }
}

void CRenderManager::renderInMenu() {
  if (!ImGui::TreeNode("Render Manager..."))
    return;

  // Stats...
  if (ImGui::TreeNode("Stats")) {
    for (size_t i = 0; i < draw_calls_per_channel.size(); ++i) {
      if (!draw_calls_per_channel[i])
        continue;
      ImGui::Text("%d / %d %s", draw_calls_per_channel[i], total_draw_calls_per_channel[i], render_channel_names.nameOf((eRenderChannel)i));
    }
    ImGui::TreePop();
  }


  if (ImGui::TreeNode("Keys")) {
    keys.renderInMenu();
    ImGui::TreePop();
  }

  ImGui::TreePop();
}


/*
  
  ShadowsCasters Mat1 Mesh1
  ShadowsCasters Mat1 Mesh1
  ShadowsCasters Mat1 Mesh2
  ShadowsCasters Mat2 Mesh3

  Solids
  Solids
  Solids
  Solids
  Solids
  Solids

  Transparents
  Transparents
  Transparents


*/