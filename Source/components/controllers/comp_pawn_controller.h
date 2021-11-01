#pragma once

#include "components/common/comp_base.h"
#include "entity/entity.h"
#include "PxPhysicsAPI.h"
#include "components/messages.h"

class TCompPawnController;
class TCompCollider;
class CEntity;

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

class TCompPawnController : public TCompBase
{
	DECL_SIBLING_ACCESS();

private:
	// Forces and impulses
	VEC3 applied_force			= VEC3::Zero;		// Used to apply forces if necessary on update
	float force_timer			= 0.f;				// To control the lerp between the force and the null vector

	void setWeaponInAreaDelay(bool status);			// To enable or disable weapons when in area delay
	void removeSlowDebuff();

protected:
	// Cached handles common to all pawns
	CHandle h_render;
	CHandle h_collider;
	CHandle h_weapon;
	std::vector<CHandle> h_weapons;						// To support multiple weapons
	CHandle h_transform;
	CHandle h_game_manager;
	std::vector<CHandle> h_weapon_colliders;

public:

	/////////////////////////////////////////////
	////////////// Pawn properties //////////////
	/////////////////////////////////////////////
	bool GOD_MODE = false;

	// Sensing
	float fov_rad = (float)M_PI / 2.f;
	float sight_radius = 12.f;
	float hearing_radius = 4.f;
	float hear_speed_th = 3.f;

	// Movement
	bool is_falling = false;
	VEC3 move_dir;

	// Area/Wave delay
	bool slow_debuf = false;
	float speed_multiplier	= 1.0f;
	float wave_delay_time	= 0.f;

	void load(const json& j, TEntityParseContext& ctx);
	void update(float dt);
	void onEntityCreated();
	void onOwnerEntityCreated(CHandle owner);

	// Weapon-related methods
	void setWeaponStatus(bool status, int current_action = 0);
	void setWeaponPartStatus(bool status, int current_action = 0);
	void setWeaponStatusAndDamage(bool status, int current_damage = 0); 
	CEntity* getWeaponEntity() { return h_weapon; }
	// TO REVISE: We said all info related to enemy attacks would be contained inside bt tasks,
	// if thats the case, enemies now use setWeaponStatusAndDamage to specifically set the current
	// damage for the weapon in each particular task. This allows for more granularity in the balance
	// but complicates making big changes to the overall damage output of the enemy

	void initPhysics(TCompCollider* comp_collider);
	void movePhysics(VEC3 moveDir, float dt, float gravity = -7.5f);
	
	// Returns true if falling
	bool manageFalling(float speed, float dt);
	void setFalling(bool is_falling) { this->is_falling = is_falling; };
	bool isFalling() { return is_falling; };
	bool isPlayerDashing();

	// Used for Area Delay (This should be also on movable objects, such as doors or platforms)
	void setSpeedMultiplier(float speed_multiplier);
	void resetSpeedMultiplier();
	bool inAreaDelay();

	// Methods called to respond to messages
	void onNewWeapon(const TMsgRegisterWeapon& msg);
	void onApplyAreaDelay(const TMsgApplyAreaDelay& msg);
	void onRemoveAreaDelay(const TMsgRemoveAreaDelay& msg);
	void onDeath(const TMsgPawnDead& msg);
	void onHitSound(const TMsgHit& msg);

	// Messages subscription
	static void registerMsgs() {
		DECL_MSG(TCompPawnController, TMsgApplyAreaDelay, onApplyAreaDelay);
		DECL_MSG(TCompPawnController, TMsgRemoveAreaDelay, onRemoveAreaDelay);
		DECL_MSG(TCompPawnController, TMsgPawnDead, onDeath);
		DECL_MSG(TCompPawnController, TMsgHit, onHitSound);
	}
};        
