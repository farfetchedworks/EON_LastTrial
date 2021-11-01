#pragma once

#include "resources/resources.h"
#include "particle.h"
#include "utils/track.h"

namespace particles
{
    enum class TEmitterType
    {
        Box = 0,
        Circle,
        CirclePerimeter,
        Sphere
    };

    class CEmitter : public IResource
    {
    public:
        int maxParticles = 1;
        int numParticles = 1;
        float interval = 0.f;
        TEmitterType type = TEmitterType::Box;
        std::variant<float, VEC3> typeSize;

        VEC3 velocity = VEC3::Zero;
        bool explode = false;
        float gravityFactor = 0.f;
        bool followOwner = false;
        
        float lifetime = 0.f;
        float lifetimeVariation = 0.f;
        
        const CTexture* texture = nullptr;
        TTrack<float> opacity;
        TTrack<VEC3> color;
        TTrack<float> size;
        bool additive = false;

        VEC2 frameSize = VEC2::One;
        int frameInitial = 0;
        int frameFinal = 0;
        int frameRate = 0;
        
        bool parse(const json& jFile);
        TParticle generate(MAT44 ownerTransform) const;
        VEC3 generatePosition() const;
        bool renderInMenu() const override;
    };
}
