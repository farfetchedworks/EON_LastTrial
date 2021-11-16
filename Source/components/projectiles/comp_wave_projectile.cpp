#include "mcv_platform.h"
#include "comp_wave_projectile.h"
#include "engine.h"
#include "modules/module_physics.h"
#include "modules/game/module_fluid_simulation.h"
#include "components/common/comp_transform.h"
#include "components/common/comp_light_point.h"
#include "render/draw_primitives.h"
#include "audio/module_audio.h"

extern CShaderCte<CtesWorld> cte_world;
extern CShaderCte<CtesAreaDelay> cte_area_delay;

DECL_OBJ_MANAGER("wave_projectile", TCompWaveProjectile)

void TCompWaveProjectile::load(const json& j, TEntityParseContext& ctx)
{
	speed_reduction = j.value("speed_reduction", speed_reduction);
}

void TCompWaveProjectile::setParameters(float wave_duration, float casting_radius, float max_casting_radius, bool byPlayer)
{
	duration = wave_duration;
	radius = casting_radius;
	max_radius = max_casting_radius;
	wave_caster = byPlayer ? EWaveCaster::PLAYER : EWaveCaster::ENEMY;
	
	changeWaveRadius(casting_radius);
}

void TCompWaveProjectile::onEntityCreated()
{
	h_transform = get<TCompTransform>();
	light_source = get<TCompLightPoint>();
	TCompLightPoint* c_light = light_source;
	light_source_intensity = c_light->intensity;
	current_time = 0.f;

	TCompTransform* comp_transform = h_transform;
	fluid_id = EngineFluidSimulation.addFluid(comp_transform->getPosition());

	EngineAudio.postEvent("CHA/General/AT/Area_Delay_Wave", comp_transform->getPosition());
}

void TCompWaveProjectile::renderAll(CTexture* diffuse)
{
	CGpuScope gpu_scope("renderWaveDelays");

	cte_area_delay.activate();

	auto mesh = Resources.get("data/meshes/AreaSphere.mesh")->as<CMesh>();
	mesh->activate();

	auto pipeline = Resources.get("area_delay.pipeline")->as<CPipelineState>();
	auto normals = Resources.get("data/textures/area_delay_normals.dds")->as<CTexture>();

	diffuse->activate(TS_ALBEDO);
	normals->activate(TS_NORMAL);

	pipeline->activate();

	getObjectManager<TCompWaveProjectile>()->forEach([&](TCompWaveProjectile* wave_delay) {
		wave_delay->activate();
		mesh->render();
	});
}

void TCompWaveProjectile::update(float dt)
{
	TCompTransform* transform = h_transform;

	current_time += dt;
	radius += dt * 6.5f;
	changeWaveRadius(radius);

	// stop expanding wave when reaching max radius
	if (radius >= max_radius)
	{
		fading = true;
	}

	TCompLightPoint* c_light = light_source;

	if(fading)
	{
		wave_intensity = damp(wave_intensity, 0.f, 3.f, dt);
		if (wave_intensity < 0.01f)
		{
			destroyWave();
			return;
		}
	}
	else
	{
		float blend_time = 0.5f;
		if (current_time <= blend_time) {
			wave_intensity = current_time;
		}
		else if (current_time > duration - blend_time) {
			wave_intensity = (duration - current_time);
		}

		wave_intensity /= blend_time;
		wave_intensity = clampf(wave_intensity, 0.0f, 1.0f);
	}

	cte_world.exposure_factor = lerp(1.f, 0.2f, wave_intensity);

	if (fluid_id != -1) {
		EngineFluidSimulation.setFluidIntensity(fluid_id, wave_intensity);
	}

	c_light->intensity = light_source_intensity * wave_intensity;

	// If someone has been debuffed, only increase radius and light effects
	if (currentDebuf.isValid())
		return;

	VHandles colliders;
	CHandle receiver = CHandle();

	physx::PxU32 flags = CModulePhysics::FilterGroup::Enemy;

	if (wave_caster == EWaveCaster::ENEMY)
		flags = CModulePhysics::FilterGroup::Player;

	if (EnginePhysics.overlapSphere(transform->getPosition(), radius, colliders, flags))
	{
		// get closer
		float max_dist = -1.f;

		for (CHandle hc : colliders)
		{
			CEntity* entity_object = hc.getOwner();
			TCompTransform* ctransform = entity_object->get<TCompTransform>();
			float dist = VEC3::DistanceSquared(ctransform->getPosition(), transform->getPosition());
			if (dist > max_dist)
			{
				max_dist = dist;
				receiver = entity_object;
			}
		}

		// Wave reaches player
		if (receiver.isValid())
		{
			assert(speed_reduction != 0);

			CEntity* entity_object = receiver;
			TMsgApplyAreaDelay msgApplyAreaDelay;
			msgApplyAreaDelay.speedMultiplier = 1.f / speed_reduction;
			msgApplyAreaDelay.isWave = true;
			msgApplyAreaDelay.waveDuration = duration;
			entity_object->sendMsg(msgApplyAreaDelay);
			currentDebuf = entity_object;
		}
	}
}

void TCompWaveProjectile::activate()
{
	// Where to render
	TCompTransform* comp_transform = h_transform;
	activateObject(comp_transform->asMatrix());

	cte_area_delay.ad_intensity = wave_intensity;
	cte_area_delay.ad_position = comp_transform->getPosition();
	cte_area_delay.ad_radius = radius;
	cte_area_delay.updateFromCPU();
}

void TCompWaveProjectile::destroyWave()
{
	EngineFluidSimulation.removeFluid(fluid_id);
	fluid_intensity = nullptr;

	// cte_world.exposure_factor = 1.0f;
	CHandle(this).getOwner().destroy();
}

void TCompWaveProjectile::changeWaveRadius(float new_radius)
{
	TCompCollider* c_collider = get<TCompCollider>();
	c_collider->setSphereShapeRadius(new_radius);

	TCompLightPoint* c_light = light_source;
	c_light->radius = new_radius;

	TCompTransform* transform = h_transform;
	transform->setScale(VEC3(new_radius));
}

void TCompWaveProjectile::debugInMenu()
{
	ImGui::DragFloat("Duration", &duration, 0.1f, 1.f, 10.f);
	bool radius_changed = ImGui::DragFloat("Radius", &radius, 5.f, 2.f, 50.f);
	if (radius_changed) changeWaveRadius(radius);
}