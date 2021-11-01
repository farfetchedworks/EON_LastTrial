#include "mcv_platform.h"
#include "particle_system.h"
#include "particle_emitter.h"
#include "render/draw_primitives.h"
#include "entity/entity.h"
#include "components/common/comp_transform.h"

extern CShaderCte<CtesParticle> cte_particle;

namespace particles
{
    CSystem::CSystem()
    {
        _pipelineCombinative = Resources.get("particle_combinative.pipeline")->as<CPipelineState>();
        _pipelineAdditive = Resources.get("particle_additive.pipeline")->as<CPipelineState>();
        _mesh = Resources.get("unit_quad_xy.mesh")->as<CMesh>();
    }

    void CSystem::setEmitter(const CEmitter* emitter)
    {
        _emitter = emitter;
    }

    void CSystem::setOwner(CHandle owner)
    {
        _owner = owner;
    }

    void CSystem::emit(float warmUpTime)
    {
        while (_particles.size() + _emitter->numParticles > _emitter->maxParticles)
        {
            _particles.pop_front();
        }

        const MAT44 ownerTransform = getOwnerTransform();

        for (int idx = 0; idx < _emitter->numParticles; ++idx)
        {
            TParticle p = _emitter->generate(ownerTransform);
            _particles.push_back(std::move(p));
        }

        warmUp(warmUpTime);

        _timeSinceEmission = 0.f;
    }

    void CSystem::warmUp(float time)
    {
        if (time <= 0.f) return;

        constexpr float kUpdateTime = 1.f / 15.f;
        while (time > 0.f)
        {
            update(kUpdateTime);
            time -= kUpdateTime;
        }
    }

    void CSystem::update(float dt)
    {
        constexpr VEC3 kGravity(0.f, -9.8f, 0.f);

        for (TParticle& particle : _particles)
        {
            particle.lifetime = particle.lifetime + dt;
            particle.velocity += kGravity * _emitter->gravityFactor * dt;
            particle.position += particle.velocity * dt;

            const float lifetimeRatio = _emitter->lifetime > 0.f ? std::max(0.f, particle.lifetime / particle.maxLifetime) : 1.f;

            particle.color = _emitter->color.get(lifetimeRatio);
            particle.opacity = _emitter->opacity.get(lifetimeRatio);
            particle.size = _emitter->size.get(lifetimeRatio);

            const int numFrames = (_emitter->frameFinal + 1) - _emitter->frameInitial;
            if (numFrames > 1)
            {
                const int totalFrames = static_cast<int>(particle.lifetime * _emitter->frameRate);
                particle.frameIdx = _emitter->frameInitial + (totalFrames % numFrames);
            }
        }

        if (_emitter->lifetime > 0.f)
        {
            auto it = std::remove_if(_particles.begin(), _particles.end(), [this](const TParticle& p) { return p.lifetime >= p.maxLifetime; });
            _particles.erase(it, _particles.end());
        }

        _timeSinceEmission += dt;

        if (_emitter->interval > 0.f && _timeSinceEmission >= _emitter->interval)
        {
            emit(0.f);
        }
    }

    void CSystem::render()
    {
        const MAT44 ownerTransform = getOwnerTransform();

        const CCamera& camera = getActiveCamera();
        const VEC3 cameraPosition = camera.getEye();
        const VEC3 cameraUp = camera.getUp();

        _emitter->additive ? _pipelineAdditive->activate() : _pipelineCombinative->activate();
        _emitter->texture->activate(TS_ALBEDO);
        _mesh->activate();

        cte_particle.activate();

        for (TParticle& particle : _particles)
        {
            const VEC3 particlePosition = _emitter->followOwner ? VEC3::Transform(particle.position, ownerTransform) : particle.position;
            const MAT44 sz = MAT44::CreateScale(particle.size);
            const MAT44 bb = MAT44::CreateBillboard(particlePosition, cameraPosition, cameraUp);
            const MAT44 world = sz * bb;

            // shader constants
            activateObject(world, Color(particle.color.x, particle.color.y, particle.color.z, particle.opacity));

            const int numFrameCols = static_cast<int>(1.f / _emitter->frameSize.x);
            const int frameRow = particle.frameIdx / numFrameCols;
            const int frameCol = particle.frameIdx % numFrameCols;

            cte_particle.particle_min_uv = VEC2(frameCol * _emitter->frameSize.x, frameRow * _emitter->frameSize.y);
            cte_particle.particle_max_uv = cte_particle.particle_min_uv + _emitter->frameSize;
            cte_particle.updateFromCPU();

            _mesh->render();
        }
    }

    MAT44 CSystem::getOwnerTransform() const
    {
        CEntity* eOwner = _owner;
        if (!eOwner) return MAT44::Identity;

        TCompTransform* cTransform = eOwner->get<TCompTransform>();
        if (!cTransform) return MAT44::Identity;

        return cTransform->asMatrix();
    }
}
