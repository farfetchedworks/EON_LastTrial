#pragma once
#include "mcv_platform.h"
#include "components/controllers/comp_pawn_controller.h"
#include "entity/entity.h"
#include "components/messages.h"
#include "components/controllers/pawn_actions.h"
#include "components/common/comp_transform.h"

namespace input { class CModule; }
using TNavPath = std::vector<VEC3>;

class TCompPlayerController : public TCompPawnController {

    DECL_SIBLING_ACCESS();

public:

    // Expose some members
    bool enabled                = true;
    bool block_attacks          = false;

    float current_speed         = 5.f;
    float heavy_attack_timer    = 0.f;
    bool can_recover_stamina    = true;         // Checked in update for stamina recovery
    int current_attack          = 0;
    int attack_count            = 0;
    bool continue_attacking     = false;
    bool continue_attacking2    = false;
    bool is_locked_on           = false;        // Lock on camera enabled
    bool is_moving              = false;        // Moving?
    bool is_sprint_attack       = false;
    bool can_enter_area_delay   = false;
    bool can_exit_area_delay    = false;
    bool is_turn_sprint         = false;
    bool is_aiming              = false;

    VEC3 move_dir               = VEC3::Zero;
    VEC3 last_dir               = VEC3::Zero;
    VEC3 raw_dir                = VEC3::Zero;   // Input dir
    float dash_force            = 0.f;
    float distance_to_ground    = 0.f;
    bool is_dashing             = false;        // Dashing?
    bool is_dash_strike         = false;
    VEC3 dash_dir               = VEC3::Zero;

    CHandle h_locked_transform;                 // For lock on camera
    CHandle last_camera;

private:

    // Water managing
    std::string owner_name;
    const CPhysicalMaterial* lastFloorMat = nullptr;
    float floorDistance = 1e6f;
    VHandles water_entities;

    // Cached handlers
    CHandle h_time_reversal;
    CHandle h_player_stamina;
    CHandle h_player_warp_energy;
    CHandle h_collider;

    float sprint_redux_freq         = 0.3f;         // Sprint stamina reduction frequency
    float attack_range              = 1.5f;
    float moving_timer              = 0.f;

    bool can_sprint                 = false;        // Can sprint? if shift key is pressed
    bool is_sprinting               = false;        // Sprinting?
    bool can_continue_sprinting     = false;        // Can continue sprinting? if the sprint key is still pressed
    const float dash_check_elapsed  = 0.3f;

    float w_height_diff             = 0.4f;
    float w_alignment               = 0.4f;
    float w_distance                = 0.2f;

    float start_sprint_time         = 0.0f;         // Time saved to calculate sprint stamina reduction
    float rot_factor                = 0.0f;         // Used for player/camera orbit & movement: 0 when front and back. -1 when left, 1 when right
    bool allow_time_reversal        = true;
    float reversal_start_timer      = 0.f;
    float reversal_timer_max        = 0.25f;

    float target_speed              = 0.f;
    float prev_target_speed         = 0.f;
    float move_speed                = current_speed;			// Regular speed
    float walk_speed                = current_speed * 0.5f;		// Walk speed
    float rotation_speed            = 5.f;
    float sprint_speed              = 6.65f;					// Sprint speed
    float speed_mod                 = 0.4f;						// Speed modifier used when healing/area delay
    float speed_multiplier_move     = 1.0f;

    bool _aimLocked                 = false;
    bool _lastIsMoving              = false;
    VEC2 _lastSpeed                 = VEC2::Zero;

    QUAT new_rot                    = QUAT::Identity;
    QUAT current_rot                = QUAT::Identity;
    
    int max_combo_attacks           = 4;              // Maximum number of hits that comprise a string of regular attacks

    // Navmesh stuff
    TNavPath currentPath;
    int pathIndex = 0;
    float pathSpeed = 1.6f;
    float pathAcceptanceDistance = 0.15f;
    float currentPathSpeed = 0.f;
    std::function<void()> pathCallback = nullptr;

    bool toggleFlyover(bool last_enabled);
    void manageLockOn();
    void manageAimCamera();
    void castArea(bool isShooterCamerEnabled);
    void sprint(float dt);
    void execEffectiveParry(const TMsgHit& msg);
    
    VEC2 getBlendSpeed();
    bool canSeeTarget(TCompTransform* camera_trans, CEntity* e_target);

    // Events
    void onHit(const TMsgHit& msg);
    void onLockedTargetDied(const TMsgEnemyDied& msg);
    void onEonRevive(const TMsgEonRevive& msg);
    void onShrineRestore(const TMsgRestoreAll& msg);
    void onTimeReversalStopped(const TMsgTimeReversalStoped& msg);
    void onStopAiming(const TMsgStopAiming& msg);
    void onApplyAreaDelay(const TMsgApplyAreaDelay& msg);
    void onRemoveAreaDelay(const TMsgRemoveAreaDelay& msg);

public:
    void load(const json& j, TEntityParseContext& ctx);
    void onEntityCreated();
    void update(float dt);
    void debugInMenu();
    void renderDebug();

    void blendCamera(const std::string& cameraName, float blendTime = 0.f, const interpolators::IInterpolator* interpolator = nullptr);
    bool isCameraEnabled(const std::string& cameraName);
    bool isMoving(bool only_pressing = false);

    // Water managing
    void setFloorMaterialInfo(const CPhysicalMaterial* material, VEC3 hitPos);
    bool inWater();

    void move(float dt);
    void resetMoveTimer();
    void removeLockOn(bool recenter = true);
    void blockAim() { _aimLocked = true; };
    void unBlockAim() { _aimLocked = false; };
    void lockOnToTarget(CHandle target);
    void setDashAnim();
    void setVariable(const std::string& name, fsm::TVariableValue value);
    void setPathSpeed(float speed);

    float getRotFactor() { return rot_factor; }
    float getSpeed() { return current_speed; }
    VEC3 getDashForce(float speed_multiplier) { return dash_dir * dash_force * speed_multiplier; }
    VEC3 getMoveDirection(bool& moving);
    CHandle getLockedTransform() { return h_locked_transform; };
    float getAttackRange() { return attack_range; }
    int  getNextAttack(bool force_next = false);
    fsm::TVariableValue getVariable(const std::string& name);

    void changeLockedTarget(VEC2& dir);
    void calcMoveDirection();
    void resetAttackCombo();
    bool moveTo(VEC3 position, float speed = 1.6f, float acceptance = 0.15f, std::function<void()> cb = nullptr);
    void updatePath(float dt);

    bool hasEnoughWarpEnergy(int value);
    bool hasMaxStamina();
    bool hasStamina();
    bool checkDashInput();
    bool checkSprintInput();

    // Messages subscription
    static void registerMsgs() {
        DECL_MSG(TCompPlayerController, TMsgRegisterWeapon, onNewWeapon);
        DECL_MSG(TCompPlayerController, TMsgHit, onHit);
        DECL_MSG(TCompPlayerController, TMsgEnemyDied, onLockedTargetDied);
        DECL_MSG(TCompPlayerController, TMsgEonRevive, onEonRevive);
        DECL_MSG(TCompPlayerController, TMsgRestoreAll, onShrineRestore);
        DECL_MSG(TCompPlayerController, TMsgTimeReversalStoped, onTimeReversalStopped);
        DECL_MSG(TCompPlayerController, TMsgStopAiming, onStopAiming);
        DECL_MSG(TCompPlayerController, TMsgApplyAreaDelay, onApplyAreaDelay);
        DECL_MSG(TCompPlayerController, TMsgRemoveAreaDelay, onRemoveAreaDelay);
    }
};
