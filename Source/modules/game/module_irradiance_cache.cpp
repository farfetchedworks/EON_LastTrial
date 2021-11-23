#include "mcv_platform.h"
#include "module_irradiance_cache.h"
#include "components/render/comp_irradiance_cache.h"
#include "components/common/comp_name.h"

extern CShaderCte<CtesWorld> cte_world;

bool CModuleIrradianceCache::start()
{
    return true;
}

bool CModuleIrradianceCache::customStart()
{
    if (started)
        return true;

    getObjectManager<TCompIrradianceCache>()->forEach([&](TCompIrradianceCache* irradiance) {
        irradiance->initCache(idx, &sh_buffers[idx]);
        idx++;
    });

    started = true;

    return true;
}

void CModuleIrradianceCache::restart()
{
    for (int cache_idx = 0; cache_idx < idx; cache_idx++)
    {
        CGPUBuffer* b = sh_buffers[cache_idx];
        assert(b);

        SAFE_DESTROY(b)

        // Restart world ctes
        cte_world.sh_data[cache_idx].sh_grid_delta = VEC3();
        cte_world.sh_data[cache_idx].sh_grid_dimensions = VEC3();
        cte_world.sh_data[cache_idx].sh_grid_start = VEC3();
        cte_world.sh_data[cache_idx].sh_grid_end = VEC3();
        cte_world.sh_data[cache_idx].sh_grid_num_probes = 0;
    }

    idx = 0;

    getObjectManager<TCompIrradianceCache>()->forEach([&](TCompIrradianceCache* irradiance) {
        irradiance->initCache(idx, &sh_buffers[idx]);
        idx++;
    });
}

void CModuleIrradianceCache::renderInMenu()
{
    if (ImGui::TreeNode("Irradiance Cache")) {

        if (ImGui::SmallButton("Calculate All Irradiance")) {
            int idx = 0;
            getObjectManager<TCompIrradianceCache>()->forEach([&](TCompIrradianceCache* irradiance) {
                irradiance->renderProbeCubemaps(idx, sh_buffers[idx]);
                idx++;
            });
        }

        int idx = 0;
        getObjectManager<TCompIrradianceCache>()->forEach([&](TCompIrradianceCache* irradiance) {

            CHandle h_irradiance = irradiance;
            TCompName* c_name = irradiance->get<TCompName>();

            if (ImGui::TreeNode(c_name->getName())) {

                ImGui::PushID(irradiance);
                irradiance->showInMenu(idx, sh_buffers[idx]);
                ImGui::PopID();

                ImGui::Separator();

                ImGui::TreePop();
            }

            idx++;
        });

        ImGui::TreePop();
    }
}
