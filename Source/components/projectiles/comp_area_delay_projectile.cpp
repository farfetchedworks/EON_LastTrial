#include "mcv_platform.h"
#include "comp_area_delay_projectile.h"
#include "engine.h"
#include "modules/module_physics.h"
#include "modules/game/module_fluid_simulation.h"
#include "components/common/comp_transform.h"
#include "components/common/comp_light_point.h"
#include "render/draw_primitives.h"
#include "audio/module_audio.h"
#include "fmod_studio.hpp"

extern CShaderCte<CtesWorld> cte_world;
extern CShaderCte<CtesAreaDelay> cte_area_delay;

DECL_OBJ_MANAGER("area_delay_projectile", TCompAreaDelayProjectile)

void TCompAreaDelayProjectile::setParameters(float area_duration, float casting_radius, float speed_reduction)
{
	this->area_duration = area_duration;
	this->radius = casting_radius;
	this->speed_reduction = speed_reduction;

	//Change the radius of the collider according to the radius set by the caster
	changeSphereRadius(casting_radius);
}

void TCompAreaDelayProjectile::applyAreaDelay(const TMsgEntityTriggerEnter& msg)
{
	CEntity* entity_object = msg.h_entity;
	TMsgApplyAreaDelay msgApplyAreaDelay;
	assert(speed_reduction != 0);
	msgApplyAreaDelay.speedMultiplier = 1 / speed_reduction;
	msgApplyAreaDelay.h_projectile = CHandle(this).getOwner();
	entity_object->sendMsg(msgApplyAreaDelay);
}

void TCompAreaDelayProjectile::removeDelayEffect(const TMsgEntityTriggerExit& msg)
{
	CEntity* entity_object = msg.h_entity;
	// dbg("entity: %s", entity_object->getName());
	TMsgRemoveAreaDelay msgRemoveAreaDelay;
	msgRemoveAreaDelay.h_projectile = CHandle(this).getOwner();
	entity_object->sendMsg(msgRemoveAreaDelay);
}

void TCompAreaDelayProjectile::onEntityCreated()
{
	projectile_transform = get<TCompTransform>();
	light_source = get<TCompLightPoint>();
	TCompLightPoint* c_light = light_source;
	light_source_intensity = c_light->intensity;
	current_time = 0.f;

	TCompTransform* comp_transform = projectile_transform;
	fluid_id = EngineFluidSimulation.addFluid(comp_transform->getPosition());

	// FMOD event
	fmod_event = EngineAudio.postEventGetInst("CHA/General/AT/Area_Delay_NEW", comp_transform->getPosition());
}

void TCompAreaDelayProjectile::renderAll(CTexture* diffuse)
{
	CGpuScope gpu_scope("renderAreaDelays");

	cte_area_delay.activate();

	auto mesh = Resources.get("data/meshes/AreaSphere.mesh")->as<CMesh>();
	mesh->activate();

	auto pipeline = Resources.get("area_delay.pipeline")->as<CPipelineState>();
	auto normals = Resources.get("data/textures/area_delay_normals.dds")->as<CTexture>();

	diffuse->activate(TS_ALBEDO);
	normals->activate(TS_NORMAL);

	pipeline->activate();

	getObjectManager<TCompAreaDelayProjectile>()->forEach([&](TCompAreaDelayProjectile* area_delay) {
		area_delay->activate();
		mesh->render();
	});
}

void TCompAreaDelayProjectile::update(float dt)
{
	current_time += dt;

	TCompLightPoint* c_light = light_source;

	float blend_time = 0.5f;
	if (current_time <= blend_time) {
		area_intensity = current_time;
	} else if (current_time > area_duration - blend_time) {
		area_intensity = (area_duration - current_time);
	}

	area_intensity /= blend_time;
	area_intensity = clampf(area_intensity, 0.0f, 1.0f);

	cte_world.exposure_factor = lerp(1.f, 0.2f, area_intensity);

	if (fluid_id != -1) {
		EngineFluidSimulation.setFluidIntensity(fluid_id, area_intensity);
	}

	c_light->intensity = light_source_intensity * area_intensity;

	// Destroy projectile
	if (current_time > area_duration) {
		destroyAreaDelay();
	}
}

// Destroys the area delay handle and removes the effect from all enemies that are inside the area
void TCompAreaDelayProjectile::destroyAreaDelay() 
{
	EngineFluidSimulation.removeFluid(fluid_id);
	fluid_intensity = nullptr;

	TCompTransform* comp_transform = projectile_transform;
	VHandles objects_in_area_delay;
	TCompCollider* comp_collider = get<TCompCollider>();

	physx::PxU32 layer_mask = CModulePhysics::FilterGroup::Enemy | CModulePhysics::FilterGroup::Projectile | CModulePhysics::FilterGroup::Door | CModulePhysics::FilterGroup::Player;
	
	if (EnginePhysics.overlapSphere(comp_transform->getPosition(), radius, objects_in_area_delay, layer_mask)) {
		for (CHandle collider_handle : objects_in_area_delay) {
			CHandle h_owner = collider_handle.getOwner();
			CEntity* entity_object = h_owner;
			if (!CHandle(entity_object).isValid()) continue;

			TMsgRemoveAreaDelay msgRemoveAreaDelay;
			msgRemoveAreaDelay.h_projectile = CHandle(this).getOwner();
			entity_object->sendMsg(msgRemoveAreaDelay);

		}
	}

	// destroy FMOD event
	fmod_event->setParameterByName("Area_Delay_End", 1.f);
	fmod_event->release();

	cte_world.exposure_factor = 1.0f;
	CHandle(this).getOwner().destroy();
}

// Change the radius of the sphere collider when the radius is changed from elsewhere
void TCompAreaDelayProjectile::changeSphereRadius(float new_radius)
{
	TCompCollider* c_collider = get<TCompCollider>();
	c_collider->setSphereShapeRadius(new_radius);
}

void TCompAreaDelayProjectile::debugInMenu()
{
	ImGui::DragFloat("Area duration", &area_duration, 0.1f, 1.f, 10.f);
	bool radius_changed = ImGui::DragFloat("Radius", &radius, 5.f, 2.f, 50.f);
	ImGui::DragFloat("Speed reduction", &speed_reduction, 5.f, 2.f, 50.f);
	if (radius_changed) changeSphereRadius(radius);
}

void TCompAreaDelayProjectile::renderDebug()
{
#ifdef _DEBUG
	TCompTransform* trans = get<TCompTransform>();
	if (!trans)
		return;

	VEC3 pos = trans->getPosition();
	drawProgressBar3D(pos, Colors::Orange, current_time, area_duration);
#endif
}

void TCompAreaDelayProjectile::activate()
{
	// Where to render
	TCompTransform* comp_transform = projectile_transform;
	activateObject(comp_transform->asMatrix());

	cte_area_delay.ad_intensity = area_intensity;
	cte_area_delay.ad_position = comp_transform->getPosition();
	cte_area_delay.ad_radius = radius;
	cte_area_delay.updateFromCPU();
}
