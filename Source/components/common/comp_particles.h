#pragma once

#include "comp_base.h"
#include "particles/particle_system.h"

struct TCompParticles : public TCompBase
{
    void load(const json& j, TEntityParseContext& ctx);

    void onEntityCreated();
    void update(float dt);
    void render();
    void debugInMenu();

    particles::CSystem _particleSystem;
    bool _autoplay = false;
    float _warmup = 0.f;
};
