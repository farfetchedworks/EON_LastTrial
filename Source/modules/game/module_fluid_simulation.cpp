#include "mcv_platform.h"
#include "module_fluid_simulation.h"
#include "entity/entity_parser.h"

extern CShaderCte<CtesWorld> cte_world;

int CModuleFluidSimulation::addFluid(VEC3 position)
{
    int index = -1;
    for (int i = 0; i < MAX_FLUID_COUNT; ++i) {
        if (ctes_fluid_sim.fs_data[i].fs_enabled == 0) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        assert(false);
        return -1;
    }

    CEntity* fluid_sim = spawn("data/prefabs/fluid_simulation.json", {});
    TCompBuffers* c_buffers = fluid_sim->get<TCompBuffers>();
    CTexture* texture = c_buffers->getTextureByName("fluid_texture");

    fluid_instances[index].fluid_sim_handle = fluid_sim;
    fluid_instances[index].fluid_texture = texture;
    ctes_fluid_sim.fs_data[index].fs_position = position;
    ctes_fluid_sim.fs_data[index].fs_size = 15;
    ctes_fluid_sim.fs_data[index].fs_enabled = 1;
    ctes_fluid_sim.fs_data[index].fs_intensity = 0.0;

    return index;
}

void CModuleFluidSimulation::removeFluid(int index)
{
    ctes_fluid_sim.fs_data[index].fs_enabled = 0;

    if (fluid_instances[index].fluid_sim_handle.isValid()) {
        fluid_instances[index].fluid_sim_handle.destroy();
    }
}

void CModuleFluidSimulation::setFluidIntensity(int index, float intensity)
{
    ctes_fluid_sim.fs_data[index].fs_intensity = intensity;
}

void CModuleFluidSimulation::activateFluids()
{
    for (int i = 0; i < MAX_FLUID_COUNT; ++i) {
        if (ctes_fluid_sim.fs_data[i].fs_enabled == 1) {
            fluid_instances[i].fluid_texture->activateVS(TS_FLUID_SIMULATION0 + i);
            fluid_instances[i].fluid_texture->activateCS(TS_FLUID_SIMULATION0 + i);
        }
    }

    ctes_fluid_sim.updateFromCPU();
    ctes_fluid_sim.activate();
    ctes_fluid_sim.activateCS();
}

void CModuleFluidSimulation::deactivateFluids()
{
    for (int i = 0; i < MAX_FLUID_COUNT; ++i) {
        CTexture::deactivateVS(TS_FLUID_SIMULATION0 + i);
        CTexture::deactivateCS(TS_FLUID_SIMULATION0 + i);
    }
}

bool CModuleFluidSimulation::start()
{
    ctes_fluid_sim.create(CB_SLOT_FLUID_SIM, "fluid_sim");

    for (int i = 0; i < MAX_FLUID_COUNT; ++i) {
        ctes_fluid_sim.fs_data[i] = {};
    }

    return true;
}

void CModuleFluidSimulation::update(float dt)
{
    //for (int i = 0; i < fluids_count; ++i) {
    //    auto& fluid_data = ctes_fluid_sim.fs_data[fluids_count];
    //    if (fluid_data.fs_enabled == 1) {
    //        fluid_data.fs_intensity += dt;
    //    }
    //}
}
