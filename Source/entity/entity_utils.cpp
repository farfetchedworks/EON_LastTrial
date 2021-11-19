#include "mcv_platform.h"

#include "handle/handle_def.h"
#include "../bin/data/shaders/constants_particles.h"
#include "entity/entity_parser.h"
#include "components/common/comp_transform.h"

CHandle spawnParticles(const std::string& name, VEC3 position, VEC3 owner_position, float radius, int iterations, int num_particles)
{
    TEntityParseContext ctx;
    for (int i = 0; i < iterations; ++i) {

        CTransform t;
        t.fromMatrix(MAT44::CreateRotationY(Random::range((float)-M_PI, (float)M_PI)) *
            MAT44::CreateTranslation(position));

        spawn(name, t, ctx);

        for (auto h : ctx.entities_loaded) {
            CEntity* e = h;
            TCompBuffers* buffers = e->get<TCompBuffers>();
            CShaderCte< CtesParticleSystem >* cte = static_cast<CShaderCte<CtesParticleSystem>*>(buffers->getCteByName("CtesParticleSystem"));
            cte->emitter_radius = radius;
            cte->emitter_initial_pos = position;
            cte->emitter_owner_position = owner_position;

            if (num_particles != -1)
                cte->emitter_num_particles_per_spawn = num_particles;

            cte->updateFromCPU();

            TCompTransform* hTrans = e->get<TCompTransform>();
            hTrans->fromMatrix(MAT44::CreateRotationY(Random::range((float)-M_PI, (float)M_PI)) *
                MAT44::CreateTranslation(hTrans->getPosition()));
        }
    }

    return !ctx.all_entities_loaded.empty() ? ctx.all_entities_loaded[0] : CHandle();
}