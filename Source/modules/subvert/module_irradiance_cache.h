#pragma once

#include "modules/module.h"
#include "render/compute/gpu_buffer.h"

#define MAX_IRRADIANCE_CACHES 2

class CModuleIrradianceCache : public IModule
{
    bool started = false;
    CGPUBuffer* sh_buffers[MAX_IRRADIANCE_CACHES] = {};

public:
    CModuleIrradianceCache(const std::string& name) : IModule(name) {}

    bool start() override;
    void update(float dt) override;
    void renderInMenu();
};
