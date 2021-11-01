#include "mcv_platform.h"
#include "engine.h"
#include "entity/entity_parser.h"
#include "modules/module_boot.h"
#include "render/gpu_culling_module.h"
#include "comp_instancing.h"
#include "entity/entity.h"
#include "comp_transform.h"
#include "comp_render.h"
#include "comp_fsm.h"
#include "skeleton/comp_skeleton.h"
#include "components/common/comp_collider.h"
#include "comp_aabb.h"

DECL_OBJ_MANAGER("gpu_instancing", TCompInstancing);

std::map<std::string, CHandle> TCompInstancing::prefabs;
VHandles TCompInstancing::instanced_prefabs;

void TCompInstancing::load(const json& j, TEntityParseContext& ctx)
{
    prefab_name = ctx.filename;
}

void TCompInstancing::onEntityCreated()
{
    // Don't create prefabs in preload
    if (Boot.isPreloading())
        return;

    CEntity* owner = CHandle(this).getOwner();
    bool is_ok = true;

    // Do instancing IF (entity has transform and has to be rendered)
    is_ok &= (owner->get<TCompRender>()).isValid();
    is_ok &= (owner->get<TCompTransform>()).isValid();
    is_ok &= (owner->get<TCompAbsAABB>()).isValid();

    // Don't do instancing IF (player, enemies)
    is_ok &= !(owner->get<TCompSkeleton>()).isValid();
    is_ok &= !(owner->get<TCompFSM>()).isValid();
    is_ok &= !(owner->get<TCompCollider>()).isValid();

    assert(is_ok);

    if (!is_ok)
        return;

    setInstancedPrefab();
}

void TCompInstancing::onAllEntitiesCreated(const TMsgAllEntitiesCreated& msg)
{
    for (auto h_prefab : instanced_prefabs)
        h_prefab.destroy();
    instanced_prefabs.clear();
}

CHandle TCompInstancing::getPrefab()
{
    auto it = prefabs.find(prefab_name);
    if (it != prefabs.end()) {
        return it->second;
    }

    return CHandle();
}

void TCompInstancing::setInstancedPrefab()
{
    CHandle handle = CHandle(this).getOwner();
    CEntity* e_handle = handle; 
    TCompAbsAABB* aabb = e_handle->get<TCompAbsAABB>();
    TCompTransform* transform = e_handle->get<TCompTransform>();

    bool casts_shadows = false;
    TCompRender* cr = e_handle->get<TCompRender>();
    for (auto& p : cr->draw_calls) {
        casts_shadows |= (p.material->getShadowsMaterial() != nullptr);
    }

    // store loaded to destroy later
    instanced_prefabs.push_back(handle);

    CHandle prefab = getPrefab();

    if (prefab.isValid()) {
        handle = prefab;
    } else {
        prefabs[prefab_name] = handle;
    }

    // Register object & matrix to be rendered
    EngineCulling.addToRender(handle, *aabb, transform->asMatrix());

    if (casts_shadows) {
        EngineCullingShadows.addToRender(handle, *aabb, transform->asMatrix());
    }
}