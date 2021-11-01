#include "mcv_platform.h"
#include "engine.h"
#include "components/abilities/comp_area_delay.h"
#include "entity/entity_parser.h"
#include "modules/module_physics.h"
#include "components/common/comp_transform.h"
#include "components/cameras/comp_camera_shooter.h"
#include "components/stats/comp_warp_energy.h"
#include "components/projectiles/comp_area_delay_projectile.h"
#include "components/projectiles/comp_wave_projectile.h"
#include "skeleton/comp_attached_to_bone.h"

DECL_OBJ_MANAGER("area_delay", TCompAreaDelay)

void TCompAreaDelay::load(const json& j, TEntityParseContext& ctx)
{
	warp_consumption = j.value("warp_consumption", warp_consumption);
	area_duration = j.value("area_duration", area_duration);
	area_radius = j.value("area_radius", area_radius);
	speed_reduction = j.value("speed_reduction", speed_reduction);
	ad_ball = j.value("ad_ball", ad_ball);
}

void TCompAreaDelay::onEntityCreated()
{
	h_transform = get<TCompTransform>();
	in_interpolator = &interpolators::expoInInterpolator;
	out_interpolator = &interpolators::expoOutInterpolator;
}

void TCompAreaDelay::update(float dt)
{
	// Case for enemies
	if (!ad_ball)
		return;

	CEntity* e = h_plasma;
	if (!e)
		return;

	// Update scale
	TCompAttachedToBone* socket = e->get<TCompAttachedToBone>();
	if (socket)
	{
		ad_ball_timer += dt;

		if (is_ball_fading)
		{
			float f = ad_ball_timer / 0.4f;
			socket->local_t.setScale(VEC3::Lerp(VEC3(0.15f), VEC3::Zero, out_interpolator->blend(0.f, 1.f, f)));
			if (f >= 1.f)
				destroyADBall();
		}
		else
		{
			float f = ad_ball_timer / 2.f;
			socket->local_t.setScale(VEC3::Lerp(socket->local_t.getScale(), VEC3(0.15f), in_interpolator->blend(0.f, 1.f, f)));

			static float ballOffsetPos = socket->offset.y;
			socket->offset.y = ballOffsetPos + 0.12f * sinf((float)Time.current * 1.75f);
		}
	}
	else
	{
		// the ball is now detached

		e->setPosition(damp<VEC3>(e->getPosition(), next_area_position, 7.f, dt));

		if (VEC3::Distance(e->getPosition(), next_area_position) <= 0.1f)
		{
			// Apply final position and scale
			CTransform transform;
			transform.setPosition(next_area_position);
			transform.setScale(VEC3(area_radius));

			// Spawn projectile
			CEntity* entity_ad_projectile = spawn("data/prefabs/ad_projectile.json", transform);
			assert(entity_ad_projectile);
			area_delay_projectile = CHandle(entity_ad_projectile);

			TCompAreaDelayProjectile* proj_adproj_comp = entity_ad_projectile->get<TCompAreaDelayProjectile>();
			proj_adproj_comp->setParameters(area_duration, area_radius, speed_reduction);

			destroyADBall();
		}
	}
}

bool TCompAreaDelay::startAreaDelay(CHandle locked_t, float duration, bool enemy_caster)
{
	float last = area_duration;
	area_duration = duration;

	bool is_ok = !enemy_caster ? startAreaDelay(locked_t) : startAreaDelayEnemy(locked_t);

	// reset last duration
	area_duration = last;
	return is_ok;
}

// Spawn an area delay in a position
bool TCompAreaDelay::startAreaDelay(CHandle locked_t)
{
	if (isActive() || isCasted()) {
		dbg("There's already an active area delay\n");
		return false;
	}

	TCompWarpEnergy* c_warp = get<TCompWarpEnergy>();
	if (!c_warp->hasWarpEnergy(warp_consumption)) {
		dbg("Not enough warp energy to cast area delay\n");
		return false;
	}

	TCompTransform* comp_transform = h_transform;

	/*
	* Case 1: Spawn in player's location
	* Case 2: Spawn in position selected by shooting camera
	*/

	// C1
	// Apply small height offset so it's not in the middle of the ground
	VEC3 position = comp_transform->getPosition() + VEC3::Up;
	
	// C2
	CEntity* e_camera = getEntityByName("camera_shooter");
	TCompCameraShooter* shooter_camera = e_camera->get<TCompCameraShooter>();

	if (shooter_camera->enabled) {

		if (is_casted)
			return false;

		TCompTransform* t_camera = e_camera->get<TCompTransform>();
		VEC3 origin = t_camera->getPosition();
		VEC3 direction = t_camera->getForward();

		physx::PxU32 layerMask = CModulePhysics::FilterGroup::Enemy | CModulePhysics::FilterGroup::Scenario;
		std::vector<physx::PxRaycastHit> raycastHits;

		bool hasHitRaycast = EnginePhysics.raycast(origin, direction, 100.f, raycastHits, layerMask, true);
		if (hasHitRaycast) {
			position = PXVEC3_TO_VEC3((raycastHits[0].position/* + raycastHits[0].normal*/));
		}
		else
		{
			// don't cast!!
			return false;
		}

		next_area_position = position;
		is_casted = true;
	}
	else
	{
		// Apply final position and scale
		CTransform transform;
		transform.setPosition(position);
		transform.setScale(VEC3(area_radius));

		// Spawn projectile
		CEntity* entity_ad_projectile = spawn("data/prefabs/ad_projectile.json", transform);
		assert(entity_ad_projectile);
		area_delay_projectile = CHandle(entity_ad_projectile);

		TCompAreaDelayProjectile* proj_adproj_comp = entity_ad_projectile->get<TCompAreaDelayProjectile>();
		proj_adproj_comp->setParameters(area_duration, area_radius, speed_reduction);
	}

	// Warp consuming
	c_warp->consumeWarpEnergy(warp_consumption);
	return true;
}

// Spawn an area delay, generated by an enemy
bool TCompAreaDelay::startAreaDelayEnemy(CHandle locked_t)
{
	if (isActive()) {
		dbg("there's already an active area delay\n");
		return false;
	}

	CTransform transform;
	TCompTransform* comp_transform = get<TCompTransform>();

	/*
	* Case 1: Spawn in enemy's location
	* Case 2: Spawn in locked player's location
	*/

	// C1
	VEC3 position = comp_transform->getPosition();

	// C2
	if (locked_t.isValid())
	{
		TCompTransform* enemy_locked_t = locked_t;
		position = enemy_locked_t->getPosition();
	}

	// Apply final position
	transform.setPosition(position);

	CEntity* entity_ad_projectile = spawn("data/prefabs/ad_projectile_enemy.json", transform);
	assert(entity_ad_projectile);
	area_delay_projectile = CHandle(entity_ad_projectile);

	// Define the parameters of the area delay projectile component
	// By design the Area Delay ability defines duration, radius and reduction
	TCompAreaDelayProjectile* proj_adproj_comp = entity_ad_projectile->get<TCompAreaDelayProjectile>();
	proj_adproj_comp->setParameters(area_duration, area_radius, speed_reduction);
	return true;
}

void TCompAreaDelay::destroyADBall(bool thrown)
{
	CEntity* e = h_plasma;
	if (!e)
		return;

	// Moving
	TCompAttachedToBone* socket = e->get<TCompAttachedToBone>();
	if (!thrown && !socket)
		return;

	if (thrown)
	{
		e->destroy();
		h_plasma = CHandle();
		ad_ball_timer = 0.f;
		is_casted = false;
		is_ball_fading = false;
	}
	else
	{
		is_ball_fading = true;
		ad_ball_timer = 0.f;
	}
}

void TCompAreaDelay::detachADBall()
{
	/*if (!h_plasma.isValid())
		return;*/
	assert(h_plasma.isValid());
	CEntity* e = h_plasma;
	e->removeComponent<TCompAttachedToBone>();
}

bool TCompAreaDelay::startWaveDelay(float initial_radius, float max_radius, float duration, bool byPlayer)
{
	// Don't consume warp energy: this is used by Gard

	if (h_active_wave.isValid())
	{
		dbg("can't start 2 delay waves\n");
		return false;
	}

	TCompTransform* myT = h_transform;

	CTransform t;
	t.setPosition(myT->getPosition() + 4 * VEC3::Up);
	t.setScale(VEC3(initial_radius));

	CEntity* entity_wave = spawn("data/prefabs/wave_projectile.json", t);
	assert(entity_wave);

	TCompWaveProjectile* waveproj_comp = entity_wave->get<TCompWaveProjectile>();
	assert(waveproj_comp);
	waveproj_comp->setParameters(duration, initial_radius, max_radius, byPlayer);

	h_active_wave = CHandle(entity_wave);
	
	return true;
}

bool TCompAreaDelay::isActive()
{
	return area_delay_projectile.isValid();
}

void TCompAreaDelay::debugInMenu()
{
	ImGui::DragInt("Warp consumption", &warp_consumption, 1, 0, 50);
	ImGui::DragFloat("Area duration", &area_duration, 0.1f, 1.f, 10.f);
	ImGui::DragFloat("Area radius", &area_radius, 5.f, 2.f, 50.f);
	ImGui::DragFloat("Speed reduction", &speed_reduction, 5.f, 2.f, 50.f);
}