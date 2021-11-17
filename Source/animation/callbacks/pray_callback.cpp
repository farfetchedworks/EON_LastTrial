#include "mcv_platform.h"
#include "entity/entity.h"
#include "engine.h"
#include "modules/game/module_player_interaction.h"
#include "modules/module_boot.h"
#include "animation/animation_callback.h"
#include "entity/entity_parser.h"
#include "../bin/data/shaders/constants_particles.h"
#include "components/common/comp_parent.h"
#include "components/gameplay/comp_lifetime.h"

extern CShaderCte<CtesWorld> cte_world;

struct onPrayCallback : public CAnimationCallback
{
	bool spawned = false;
	bool stopped = false;
	float exposure = 1.f;
	CHandle h_e;

	void AnimationUpdate(float anim_time, CalModel* model, CalCoreAnimation* animation, void* userData)
	{
		if (!Boot.inGame())
			return;

		CEntity* owner = getOwnerEntity(userData);

		if (anim_time > 2.75f && !spawned)
		{
			spawned = true;
			CEntity* shrine = PlayerInteraction.getLastShrine();
			TCompParent * shrine_parent = shrine->get<TCompParent>();
			CEntity* hole = shrine_parent->getChildByName("shrine_black_hole");
			assert(hole);

			CTransform t;
			t.setPosition(hole->getPosition());
			CEntity* e = spawn("data/particles/compute_direct_geons_particles.json", t);
			h_e = e;
			TCompLifetime* life = e->get<TCompLifetime>();
			life->setTTL(5.f);
			TCompBuffers* buffers = e->get<TCompBuffers>();
			CShaderCte< CtesParticleSystem >* cte = static_cast<CShaderCte< CtesParticleSystem >*>(buffers->getCteByName("CtesParticleSystem"));

			cte->emitter_initial_pos = hole->getPosition();
			cte->emitter_owner_position = owner->getPosition();
			cte->emitter_num_particles_per_spawn = 2500;
			cte->emitter_time_between_spawns = 0.3f;
			cte->updateFromCPU();

			exposure = 0.12f;
		}

		if (anim_time > 4.f && !stopped)
		{
			if (h_e.isValid())
			{
				CEntity* e = h_e;
				TCompBuffers* buffers = e->get<TCompBuffers>();
				CShaderCte< CtesParticleSystem >* cte = static_cast<CShaderCte< CtesParticleSystem >*>(buffers->getCteByName("CtesParticleSystem"));
				cte->emitter_num_particles_per_spawn = 0;
				cte->updateFromCPU();
				stopped = true;
			}
		}

		if (anim_time > 6.5f && exposure != 1.f)
		{
			exposure = 1.f;
		}

		cte_world.exposure_factor = damp(cte_world.exposure_factor, exposure, 2.f, Time.delta);
	}

	void AnimationComplete(CalModel* model, CalCoreAnimation* animation, void* userData)
	{
		spawned = false;
		stopped = false;
		exposure = 1.f;
		cte_world.exposure_factor = 1.f;
	}
};

REGISTER_CALLBACK("onPray", onPrayCallback)