#pragma once

#include "modules/module.h"

#define MAX_FLUID_COUNT 3

class CTexture;

class CModuleFluidSimulation : public IModule
{

    struct FluidInstance {
        CHandle fluid_sim_handle;
        CTexture* fluid_texture = nullptr;
    };

    CShaderCte<CtesFluidSim> ctes_fluid_sim;
    FluidInstance fluid_instances[MAX_FLUID_COUNT];

public:
	CModuleFluidSimulation(const std::string& name) : IModule(name) {}

    int addFluid(VEC3 position);
    void removeFluid(int index);
    void setFluidIntensity(int index, float intensity);

    void activateFluids();
    void deactivateFluids();

    bool start() override;
    void update(float dt) override;
};
