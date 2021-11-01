#pragma once

#include "components/common/comp_base.h"
#include "entity/entity.h"
#include "components/messages.h"

class TCompEnemyProjectile : public TCompBase {

	DECL_SIBLING_ACCESS();

	friend class TCompAIControllerRanged;			// Area delay component must define the parameters of the projectile

private:

	int damage = 10;
	float lifetime = 10.f;									// Time before the projectile gets destroyed if it doesn't hit anything in its way
	float current_time = 0.f;								// Timer to destroy the projectile
	float speed_multiplier = 1.f;							// Speed multiplier to change time scale on Area Delay
	float speed = 10.f;										// Movement speed of the projectile (it doesn't rotate)
	float duration = 0.f;									// Duration and speed reduction are used to apply area delay to Eon
	float speed_reduction = 0.f;
	bool from_player = false;
	bool is_homing = false;									// If true, the projectile will chase the player unless it hits a collision or lifetime expires

	CHandle projectile_transform;
	CHandle projectile_collider;
	CHandle target;											// Target of the projectile when chasing the player
	CHandle trail;

	VEC3 front;
	VEC3 initial_scale;

	void move(float dt);
	void destroy();

	// Called when the projectile has hit anything	
	void onHitObject(const TMsgEntityOnContact& msg);

	// Called when the projectile has hit anything	
	void onApplyAreaDelay(const TMsgApplyAreaDelay& msg);
	void onRemoveAreaDelay(const TMsgRemoveAreaDelay& msg);

public:

	static void registerMsgs() {
		DECL_MSG(TCompEnemyProjectile, TMsgEntityOnContact, onHitObject);	
		DECL_MSG(TCompEnemyProjectile, TMsgApplyAreaDelay, onApplyAreaDelay);
		DECL_MSG(TCompEnemyProjectile, TMsgRemoveAreaDelay, onRemoveAreaDelay);
	}

	void setParameters(int damage, bool from_player, VEC3 new_target, bool is_homing = false);

	void load(const json& j, TEntityParseContext& ctx);
	void onEntityCreated();
	void update(float dt);
	void debugInMenu();
};
