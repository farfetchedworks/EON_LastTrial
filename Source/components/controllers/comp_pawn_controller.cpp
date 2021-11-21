#include "mcv_platform.h"
#include "comp_pawn_controller.h"
#include "engine.h"
#include "entity/entity.h"
#include "components/common/comp_collider.h"
#include "components/common/comp_name.h"
#include "components/common/comp_transform.h"
#include "components/common/comp_render.h"
#include "components/common/comp_prefab_info.h"
#include "audio/module_audio.h"
#include "components/gameplay/comp_game_manager.h"
#include "components/controllers/pawn_utils.h"
#include "components/controllers/comp_ai_controller_base.h"
#include "components/controllers/comp_player_controller.h"
#include "components/projectiles/comp_enemy_projectile.h"
#include "components/stats/comp_health.h"
#include "components/postfx/comp_motion_blur.h"
#include "entity/entity_parser.h"
#include "modules/module_physics.h"
#include "render/draw_primitives.h"
#include "components/common/comp_fsm.h"

DECL_OBJ_MANAGER("pawn", TCompPawnController)

void TCompPawnController::load(const json& j, TEntityParseContext& ctx)
{
	// Sensing
	float fov_deg = j.value("fov_deg", 90.0f);
	fov_rad = deg2rad(fov_deg);
	sight_radius = j.value("sight_radius", sight_radius);
	hearing_radius = j.value("hearing_radius", hearing_radius);
}

void TCompPawnController::onEntityCreated()
{
	if (!GameManager)
		return;

	h_transform = get<TCompTransform>();
	h_render = get<TCompRender>();
	h_game_manager = GameManager->get<TCompGameManager>();
}

void TCompPawnController::onOwnerEntityCreated(CHandle owner)
{
	CEntity* e = owner;
	initPhysics(e->get<TCompCollider>());
}

void TCompPawnController::update(float dt)
{
	if (slow_debuf)
	{
		wave_delay_time -= dt;

		if (wave_delay_time <= 0)
			removeSlowDebuff();
	}
}

void TCompPawnController::initPhysics(TCompCollider* comp_collider)
{
	h_collider = CHandle(comp_collider);
}

void TCompPawnController::movePhysics(VEC3 moveDir, float dt, float gravity)
{
	PawnUtils::movePhysics(h_collider, moveDir, dt, gravity);

	// Update new normalized dir
	move_dir = moveDir;
	move_dir.Normalize();
}

void TCompPawnController::removeSlowDebuff()
{
	slow_debuf = false;

	// Send message to self to disable debuff
	// The reason this is communicated as a message is to also inform
	// the PlayerController (and not only PawnController) that the debuff ended
	// and thus restore FMOD audio to normal
	TMsgRemoveAreaDelay msg;
	CHandle(this).getOwner().sendMsg(msg);

	// Remove motion blur if needed
	CEntity* camera = getEntityByName("camera_mixed");
	assert(camera);
	TCompMotionBlur* c_motion_blur = camera->get<TCompMotionBlur>();
	if (c_motion_blur && c_motion_blur->isEnabled()) {
		c_motion_blur->disable();
	}
}

bool TCompPawnController::isPlayerDashing()
{
	TCompPlayerController* controller = get<TCompPlayerController>();
	return controller && controller->is_dashing;
}

bool TCompPawnController::manageFalling(float speed, float dt)
{
	// Check if i'm "flying"
	float maxDistance = 1.25f;
	float distance = -1.f;

	TCompCollider* c_collider = h_collider;
	TCompTransform* c_player_trans = h_transform;
	VEC3 base_ray_pos = c_player_trans->getPosition() + VEC3::Up;

	const int num_rays = 4;
	VEC2 ray_offsets[num_rays] = { VEC2(0.5f, 0.f), VEC2(-0.5f, 0.f), VEC2(0.f, 0.5f), VEC2(0.f, -0.5f) };

	bool is_grounded = true;
	int num_misses = 0;
	if (!c_collider->collisionAtDistance(base_ray_pos, -VEC3::Up, maxDistance, distance)) {

		VEC3 fall_dir;
		for (int i = 0; i < num_rays; ++i) {
			VEC3 sample_ray = base_ray_pos;
			sample_ray.x += ray_offsets[i].x;
			sample_ray.z += ray_offsets[i].y;

			if (!c_collider->collisionAtDistance(sample_ray, -VEC3::Up, maxDistance, distance)) {
				fall_dir += sample_ray - c_player_trans->getPosition();
				num_misses++;
			}
		}
		fall_dir.y = -0.2f;

		if (num_misses < num_rays) {
			movePhysics(fall_dir * speed * dt, dt);
		}

		is_grounded = false;
	}

	if (!is_grounded && num_misses == num_rays) {

		if (c_collider->distanceToGround() > 1.25f)
			return true;
	}

	return false;
}

void TCompPawnController::setWeaponStatus(bool status, int current_action)
{
	assert(h_weapon.isValid());
	CEntity* eWeapon = h_weapon;
	eWeapon->sendMsg(TMsgEnableWeapon({ status, current_action }));

	setWeaponPartStatus(status, current_action);
}

void TCompPawnController::setWeaponPartStatus(bool status, int current_action)
{
	for (CHandle h : h_weapon_colliders)
	{
		if (!h.isValid())
			continue;
		CEntity* eCancellAttack = h;
		eCancellAttack->sendMsg(TMsgEnableWeapon({ status, current_action }));
	}
}

void TCompPawnController::setWeaponStatusAndDamage(bool status, int current_damage)
{ 
	// If there is only one weapon, enable it
	if (h_weapons.size() == 0) {
		assert(h_weapon.isValid());
		CEntity* eWeapon = h_weapon;
		eWeapon->sendMsg(TMsgEnableWeapon({ status, 0, current_damage }));
		return;
	}

	// If there are more than one weapons, enable all of them and set their damage as the current damage divided by all weapons
	int calc_damage =  current_damage / (int)h_weapons.size();
	for (auto weapon : h_weapons) {
		CEntity* eWeapon = weapon;
		eWeapon->sendMsg(TMsgEnableWeapon({ status, 0, calc_damage }));
	}

}

/*
  To be changed: assigns the same speed to every pawn, but it has to be proportional to each one's speed
  use a multiplier
*/
void TCompPawnController::setSpeedMultiplier(float new_speed_multiplier)
{
	speed_multiplier = new_speed_multiplier;
}

void TCompPawnController::resetSpeedMultiplier()
{
	speed_multiplier = 1.f;
}

bool TCompPawnController::inAreaDelay()
{
	return speed_multiplier < 1.f;
}

// Message callbacks

void TCompPawnController::onNewWeapon(const TMsgRegisterWeapon& msg)
{
	assert(msg.h_weapon.isValid());
	if (msg.is_weapon) {
		
		// If there was no previous weapon, register it and return
		if (!h_weapon.isValid()) {
			h_weapon = msg.h_weapon;
			return;
		}
		
		// If there was a previous weapon, add it to the weapons vector
			
		// The first time, add the first registered weapon to the vector
		if (h_weapons.size() == 0) {
			h_weapons.push_back(h_weapon);
		}
			
		// Always add the new registered weapon
		h_weapons.push_back(msg.h_weapon);

	}
	else {
		h_weapon_colliders.push_back(msg.h_weapon);
	}
}

void TCompPawnController::onApplyAreaDelay(const TMsgApplyAreaDelay& msg)
{
	setSpeedMultiplier(msg.speedMultiplier);
	setWeaponInAreaDelay(true);				// Disable weapons during area delay

	if (msg.isWave)
	{
		// Alex: Eon no hace wave delays asi que en principio no hace falta comprobar
		// más cosas (de momento)

		CEntity* camera = getEntityByName("camera_mixed");
		assert(camera);
		TCompMotionBlur* c_motion_blur = camera->get<TCompMotionBlur>();
		if (c_motion_blur) {
			c_motion_blur->enable();
		}

		slow_debuf = true;
		wave_delay_time = msg.waveDuration;
	}
}

void TCompPawnController::onRemoveAreaDelay(const TMsgRemoveAreaDelay& msg)
{
	resetSpeedMultiplier();
	setWeaponInAreaDelay(false);			// Enable weapons after area delay
}

void TCompPawnController::setWeaponInAreaDelay(bool status)
{
	// If the player is the owner send the message to the player's weapon
	CEntity* e_owner = getEntity();
	if (e_owner == getEntityByName("player")) {
		TCompPlayerController* c_controller = e_owner->get<TCompPlayerController>();
		c_controller->getWeaponEntity()->sendMsg(TMsgWeaponInAreaDelay({ status }));
		return;
	}

	// In the case of the enemy, check if there are more than one weapon
	TCompAIControllerBase* c_controller = e_owner->get<TCompAIControllerBase>();
	if (h_weapons.size() == 0) {
		CEntity* e_weapon = c_controller->getWeaponEntity();
		if(c_controller && e_weapon)		// Only apply effect to weapon damage when the entity has a weapon
			e_weapon->sendMsg(TMsgWeaponInAreaDelay({ status }));
		return;
	}

	for (auto weapon : h_weapons) {
		CEntity* e_weapon = weapon;
		if (c_controller && e_weapon)		// Only apply effect to weapon damage when the entity has a weapon
			e_weapon->sendMsg(TMsgWeaponInAreaDelay({ status }));
	}
}

// Sent by a death area so that Eon sets its health to 0
void TCompPawnController::onDeath(const TMsgPawnDead& msg)
{
	TCompHealth* c_health = get<TCompHealth>();
	int current_health = c_health->getHealth();
	c_health->setHealth(0);
}

void TCompPawnController::onHitSound(const TMsgHit& msg)
{
	TCompPrefabInfo* t_info = get<TCompPrefabInfo>();
	TCompTransform* t_trans = get<TCompTransform>();
	TCompName*		t_name	= get<TCompName>();

	if (!t_trans)
		return;

	// FMOD only SLASH FLESH events, screams go separate in sync with animations
	if (t_info) {
		if (!t_info->getPrefabName()->compare("Enemy Melee Prefab")) {
			EngineAudio.postEvent("CHA/Lancer/Lancer_Light_Hitstun", t_trans->getPosition());
		}
		else if (!t_info->getPrefabName()->compare("Enemy Ranged Prefab")) {
			EngineAudio.postEvent("CHA/Lancer/Lancer_Light_Hitstun", t_trans->getPosition());
		}
		else if (!t_info->getPrefabName()->compare("Gard")) {
			EngineAudio.postEvent("CHA/Gard/Gard_Receive_Hit", t_trans->getPosition());
		}
		else if (!t_info->getPrefabName()->compare("Cygnus_Form_1")) {
			EngineAudio.postEvent("CHA/Cygnus/P1/DMG/Cygnus_P1_Dmg_Light", t_trans->getPosition());
		}
	}
	else {
		if (!std::string(t_name->getName()).compare("Cygnus_Form_2")) {
			TCompFSM* t_fsm = get<TCompFSM>();
			int phase_number = std::get<int>(t_fsm->getCtx().getVariableValue("phase_number"));

			switch (phase_number) {
			case 2:
				EngineAudio.postEvent("CHA/Cygnus/P2/DMG/Cygnus_P2_Dmg_Light", t_trans->getPosition());
				break;
			case 3:
				EngineAudio.postEvent("CHA/Cygnus/P3/DMG/Cygnus_P3_Dmg_Light", t_trans->getPosition());
				break;
			case 4:
				EngineAudio.postEvent("CHA/Cygnus/P4/DMG/Cygnus_P4_Dmg_Light", t_trans->getPosition());
				break;
			default:
				EngineAudio.postEvent("CHA/Cygnus/P3/DMG/Cygnus_P3_Dmg_Light", t_trans->getPosition());
				break;
			}
		}
	}
}
