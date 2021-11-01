#pragma once

#include "mcv_platform.h"

namespace particles
{
    struct TParticle
    {
        VEC3 position = VEC3::Zero;
        VEC3 velocity = VEC3::Zero;
        float lifetime = 0.f;
        float maxLifetime = 0.f;
        VEC3 color = VEC3::One;
        float opacity = 1.f;
        float size = 1.f;
        int frameIdx = 0;
    };
}
