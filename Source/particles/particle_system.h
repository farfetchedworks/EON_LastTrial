#pragma once

#include "particle.h"

namespace particles
{
    class CEmitter;

    class CSystem
    {
    public:
        CSystem();

        void setEmitter(const CEmitter* emitter);
        void setOwner(CHandle owner);

        void emit(float warmup);
        void warmUp(float time);
        void update(float dt);
        void render();

    private:
        const CEmitter* _emitter = nullptr;
        CHandle _owner;
        std::deque<TParticle> _particles;
        float _timeSinceEmission = 0.f;

        const CPipelineState* _pipelineCombinative = nullptr;
        const CPipelineState* _pipelineAdditive = nullptr;
        const CMesh* _mesh = nullptr;

        MAT44 getOwnerTransform() const;
    };
}
