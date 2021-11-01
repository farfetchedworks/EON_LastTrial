#include "mcv_platform.h"
#include "comp_particles.h"
#include "entity/entity_parser.h"
#include "particles/particle_emitter.h"

DECL_OBJ_MANAGER("particles", TCompParticles);

void TCompParticles::load(const json& j, TEntityParseContext& ctx)
{
    const std::string& filename = j["file"];
    const particles::CEmitter* emitter = Resources.get(filename)->as<particles::CEmitter>();
    _particleSystem.setEmitter(emitter);
    _particleSystem.setOwner(CHandle(this).getOwner());
    _autoplay = j.value("autoplay", _autoplay);
    _warmup = j.value("warmup", _warmup);
}

void TCompParticles::onEntityCreated()
{
    if (_autoplay)
    {
        _particleSystem.emit(_warmup);
    }
}

void TCompParticles::update(float dt)
{
    _particleSystem.update(dt);
}

void TCompParticles::render()
{
    _particleSystem.render();
}

void TCompParticles::debugInMenu()
{
    // ...
}

