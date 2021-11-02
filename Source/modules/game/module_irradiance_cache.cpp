#include "mcv_platform.h"
#include "module_irradiance_cache.h"
#include "components/render/comp_irradiance_cache.h"
#include "components/common/comp_name.h"

extern CShaderCte<CtesWorld> cte_world;

bool CModuleIrradianceCache::start()
{
    if (started)
        return true;

    int idx = 0;
    getObjectManager<TCompIrradianceCache>()->forEach([&](TCompIrradianceCache* irradiance) {
        irradiance->initCache(idx, &sh_buffers[idx]);
        idx++;
    });

    started = true;

    return true;
}

void CModuleIrradianceCache::update(float dt)
{

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
