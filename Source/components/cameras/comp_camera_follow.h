#pragma once

#include "components/cameras/comp_gameplay_camera.h"
#include "components/common/comp_base.h"
#include "entity/entity.h"

namespace input {
    class CModule;
}

struct TCompCameraFollow : public TCompBase, IGameplayCamera {

    DECL_SIBLING_ACCESS();

    std::string target;
    bool target_dead = false;

    float distance = 0.0f;
    float height = 0.0f;
    float lerp_scale = 5.5f;
    float time_elapsed = 0.0f;
    float time_elapsed_energey_wall = 0.0f;

    LerpedValue<float> current_distance = {};
    LerpedValue<float> current_height = {};
    LerpedValue<float> speed_lerp = {};

    LerpedValue<VEC3> target_position_lerp = {};
    LerpedValue<VEC3> collision_position_lerp = {};

    VEC3 lock_target;
    VEC3 lastTargetPosition;
    VEC3 original_pos;
    VEC3 restrict_offset = VEC3::Zero;

    CHandle h_transform;
    CHandle h_player;        // Player controller
    CHandle h_trans_target;  // Transform of camera's target
    CHandle h_trans_locked_enemy;  // Transform of camera's locked entity

    bool is_locked_on = false;
    bool must_recenter = false;

    void onEntityCreated();
    void load(const json& j, TEntityParseContext& ctx);
    void debugInMenu();
    void renderDebug();
    void update(float dt);

    void lockedMovement(float dt);
    float getLerpFactor();
    float getAngleLerpFactor();
    VEC3 getRestrictOffset(VEC3 target_pos);

    void applyAxisRestrictions(VEC3& dst);
    void setLockedEntity(CHandle hEntity);
    void reset(bool spawn_reset = false);
    VEC3 getLockedPos(CHandle h_locked_entity, float& finalHeight);
    bool checkUnlockDistance(CHandle h_locked_entity);
    bool checkUnlockDistance(CHandle h_locked_entity, VEC3& locked_pos, VEC3& camera_front);
};