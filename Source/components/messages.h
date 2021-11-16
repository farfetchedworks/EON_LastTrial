#pragma once

#include "entity/entity.h"
#include "fsm/fsm.fwd.h"

// Declare a struct/class to hold the msg arguments
// Inside add the macro DECL_MSG_ID();
// Inside each component that wants to receive that msg, add the following:
//
// static void registerMsgs() {
//   DECL_MSG(TCompName, TMsgName, myMethodName);
// }
//
// The component must implement a method with the signature:
// void myMethodName( const TMsg& msg ) { ... do whatever with msg... }
//
// The msg 'dies' after the sendMsg finishes.

struct TMsgExplosion {
  DECL_MSG_ID();
  VEC3 center;
  float radius = 5.0f;
  float damage = 10.0f;
};

struct TMsgEnemyDied {
  DECL_MSG_ID();
  CHandle h_enemy;
  int geons = 0;
  bool backstabDeath = false;
};

// Attacks & Hits

enum class EHIT_TYPE {
	DEFAULT,
	SWEEP,
	SMASH
};

struct TMsgHit {
  DECL_MSG_ID();
  CHandle h_striker;
  VEC3 position;
  VEC3 forward;
  int damage = 0;
  bool hitByPlayer = false;
  bool hitByProjectile = false;
  bool fromBack = false;
  EHIT_TYPE hitType = EHIT_TYPE::DEFAULT;
};

struct TMsgReduceHealth {
	DECL_MSG_ID();
	CHandle h_striker;
	int damage = 0;
	bool hitByPlayer = false;
	bool fromBack = false;
	bool fall_damage = false;
};

struct TMsgRegisterWeapon {
	DECL_MSG_ID();
	CHandle h_weapon;
	bool is_weapon = true;
};

struct TMsgEnableWeapon {
	DECL_MSG_ID();
	bool enabled = false;
	int action = 0;

	// cur_dmg is only relevant for enemies, since the BT tasks carry all the information regarding damage
	int cur_dmg = 0;
};

// To disable/enable the weapon when affected by area delay
struct TMsgWeaponInAreaDelay {
	DECL_MSG_ID();
	bool in_area_delay = false;
};

struct TMsgHitWarpRecover {
	DECL_MSG_ID();
	bool hitByPlayer = false;
	bool fromBack = false;
	float multiplier = 1.f;
};

struct TMsgPreHit {
	DECL_MSG_ID();
	CHandle h_striker;
	bool hitByPlayer = false;
};

struct TMsgTarget {
	DECL_MSG_ID();
};

struct TMsgUntarget {
	DECL_MSG_ID();
};

struct TMsgEnemyStunned {
	DECL_MSG_ID();
};

// ******          ******          ******

struct TMsgPlayerSeen{
	DECL_MSG_ID();
	VEC3 where;
};

struct TMsgRegisterInSquad{
	DECL_MSG_ID();
	CHandle new_member;
};

struct TMsgEntityOnContact {
  DECL_MSG_ID();
  CHandle h_entity;
};

struct TMsgEntityOnContactStay {
	DECL_MSG_ID();
	CHandle h_entity;
};

struct TMsgEntityOnContactExit {
	DECL_MSG_ID();
	CHandle h_entity;
};

struct TMsgEntityTriggerEnter {
  DECL_MSG_ID();
  CHandle h_entity;
};

struct TMsgEntityTriggerExit {
  DECL_MSG_ID();
  CHandle h_entity;
};

struct TMsgAddForce {
  DECL_MSG_ID();
  CHandle h_applier;
  VEC3 force = VEC3::Zero;
  bool byPlayer = true;
	bool disableGravity = false;
	std::string force_origin = std::string();
};

struct TMsgRemoveForces {
  DECL_MSG_ID();
  CHandle h_striker;
	bool byPlayer = true;
	std::string force_origin = std::string();
};

struct TMsgApplyAreaDelay {
  DECL_MSG_ID();
  float speedMultiplier;
  CHandle h_projectile;
  // -------------
  bool isWave = false;
  float waveDuration = 0.f;
};

struct TMsgRemoveAreaDelay {
  DECL_MSG_ID();
  CHandle h_projectile;
};

struct TMsgStopAiming {
	DECL_MSG_ID();
	float blendIn = 0.5f;
};


/*
  Messages for Game Management
*/
// Sent to game manager when eon has died
struct TMsgEonHasDied {
  DECL_MSG_ID();
  CHandle h_sender;
};

// Sent to game manager if eon has revived
struct TMsgEonHasRevived {
  DECL_MSG_ID();
};

struct TMsgPropDestroyed {
	DECL_MSG_ID();
	VEC3 direction = VEC3::Zero;
};

// Shrines

struct TMsgShrinePray {
	DECL_MSG_ID();
	VEC3 position;
	VEC3 collider_pos;
};

struct TMsgShrineCreated {
	DECL_MSG_ID();
	CHandle h_shrine;
};

struct TMsgRestoreAll {
	DECL_MSG_ID();
};

struct TMsgTimeReversalStoped {
	DECL_MSG_ID();
	float force_scale = 0.0f;
	std::string state_name;
	float time_in_anim = 0.0f;
	VEC3 linear_velocity;
};

struct TMsgBossDead {
  DECL_MSG_ID();
  std::string boss_name;
};

// Sent to Eon to allow it to revive (if possible)
struct TMsgEonRevive {
  DECL_MSG_ID();
};

// Sent to Pawns when dead (death area, etc)
struct TMsgPawnDead {
  DECL_MSG_ID();
};

// Sent to Eon to when Eon enters a death area
struct TMsgEonInTriggerArea {
  DECL_MSG_ID();
  std::string area_name = std::string();
  bool is_trigger_enter = false;
};

// Sent from a button to a door or mechanism when it gets activated
struct TMsgButtonActivated {
  DECL_MSG_ID();
};

// Sent from a button to a door or mechanism when it gets activated
struct TMsgButtonDeactivated {
  DECL_MSG_ID();
};

// Sent from the interact state when Eon acts on an interactable
struct TMsgEonInteracted {
  DECL_MSG_ID();
  CHandle h_player;
};

// Sent from the trigger area to the associated comp message area to handle the activation/deactivation
struct TMsgActivateMsgArea {
  DECL_MSG_ID();
};

// Sent from the trigger area to the associated comp message area to handle the activation/deactivation
struct TMsgDeactivateMsgArea {
  DECL_MSG_ID();
};

struct TMsgCameraCreated {
	DECL_MSG_ID();
	std::string name;
	CHandle camera;
	bool active = false;
};

struct TMsgUnregEvent {
	DECL_MSG_ID();
	std::string name;
};
// ------------------------------
struct TMsgDefineLocalAABB {
  DECL_MSG_ID();
  AABB* aabb = nullptr;
};

struct TMsgFSMVariable {
    std::string name;
    fsm::TVariableValue value;
    DECL_MSG_ID();
};

struct TMsgShake {
    float amount = 0.f;
    bool enabled = false;
    DECL_MSG_ID();
};

struct TMsgAllEntitiesCreated {
	DECL_MSG_ID();
};

struct TMsgFXToStack {
	CHandle fx;
	int priority = 0;
	DECL_MSG_ID();
};

struct TMsgRenderPostFX {
	CHandle fx;
	DECL_MSG_ID();
};

// Sent to Pawns when dead (death area, etc)
struct TMsgEnemyEnter {
	DECL_MSG_ID();
};

struct TMsgEnemyExit {
	DECL_MSG_ID();
};