#include "mcv_platform.h"
#include "engine.h"
#include "input/input_module.h"
#include "render/draw_primitives.h"
#include "modules/module_physics.h"
#include "modules/module_camera_mixer.h"
#include "components/messages.h"
#include "components/ai/comp_bt.h"
#include "components/common/comp_transform.h"
#include "components/cameras/comp_camera_follow.h"
#include "components/cameras/comp_camera_shake.h"
#include "components/controllers/comp_player_controller.h"
#include "components/common/comp_fsm.h"
#include "fsm/states/logic/state_logic.h"
#include "components/combat/comp_lock_target.h"
#include "components/stats/comp_health.h"

DECL_OBJ_MANAGER("camera_follow", TCompCameraFollow)

const float MAX_LOCKED_DISTANCE = 15.f;

void TCompCameraFollow::onEntityCreated()
{
    input = CEngine::get().getInput(input::PLAYER_1);
    assert(input);

    CEntity* owner = getEntity();
    name = owner->getName();

    TMsgCameraCreated msg;
    msg.name = name;
    msg.camera = CHandle(this);
    msg.active = true;
    GameManager->sendMsg(msg);

    enabled = true;
    lock_target = VEC3(-1e8f);

    TCompTransform* c_transform = get<TCompTransform>();
    original_pos = c_transform->getPosition();
    h_transform = c_transform;
}

void TCompCameraFollow::load(const json& j, TEntityParseContext& ctx)
{
    IGameplayCamera::load(j, ctx);

    target = j.value("target", "");
    height = j.value("height", height);
    distance = j.value("distance", distance);
    restrict_offset = loadVEC3(j, "restrict_offset", restrict_offset);
    lerp_scale = j.value("lerp_scale", lerp_scale);

    rotation_sensibility = j.value("rotation_sensibility", rotation_sensibility);
    rotation_sensibility = deg2rad(rotation_sensibility);

    orbit_speed = j.value("orbit_speed", orbit_speed);

    delta_yaw = delta_yaw_lerp = j.value("yaw", delta_yaw);
    delta_pitch = delta_pitch_lerp = j.value("pitch", delta_pitch);
}

void TCompCameraFollow::debugInMenu()
{
    ImGui::Text("Target: %s", target.c_str());
    ImGui::DragFloat3("Target LERP Pos", &target_position_lerp.x);
    ImGui::Checkbox("Enabled", &enabled);
    ImGui::Text("Axis Restrictions");
    ImGui::Checkbox("X", &restrict_x_axis);
    ImGui::Checkbox("Y", &restrict_y_axis);
    ImGui::Checkbox("Z", &restrict_z_axis);
    ImGui::Separator();
    ImGui::DragFloat("Distance", &distance, 0.02f, 1.f, 20.f);
    ImGui::DragFloat("Height", &height, 0.02f, 0.f, 20.f);
    ImGui::DragFloat3("Offset", &restrict_offset.x, 0.1f, -100.f, 100.f);
    ImGui::Text("Orbit");
    ImGui::Checkbox("Active", &orbit_enabled);
    if(orbit_enabled)
        ImGui::DragFloat("Speed", &orbit_speed, 1.f, 500.f, 3000.f);
    ImGui::Separator();
    if (orbit_enabled) {
        ImGui::DragFloat("Yaw", &delta_yaw, 0.01f, -(float)M_PI, (float)M_PI);
        ImGui::DragFloat("Pitch", &delta_pitch, 0.01f, -(float)M_PI, (float)M_PI);
    }
    else {
        ImGui::Text("Yaw %f", delta_yaw);
        ImGui::Text("Pitch %f", delta_pitch);
    }
}

void TCompCameraFollow::renderDebug()
{
    // lock on
    const TMixedCamera& camera = CameraMixer.getCameraByName("camera_follow");
    if (!enabled || camera.weight != 1.f) {
        return;
    }

    drawCircle3D(lock_target, Colors::White, 4.f);
}

void TCompCameraFollow::update(float dt)
{
    time_elapsed += dt;

    if (Time.scale_factor != 1.0 && Time.scale_factor > 0.0f) {
        dt /= (Time.scale_factor * 1.5f);
    }

    TCompTransform* c_transform = h_transform;

    // If the transform is not valid, can be either its the first time,
    // or Eon is dead
    if (!h_trans_target.isValid())
    {
        CEntity* e_target = getEntityByName(target);
          
        if (!e_target)
            return;

        h_trans_target = e_target->get<TCompTransform>();
        TCompTransform* c_trans = h_trans_target;
        target_position_lerp = c_trans->getPosition();
        target_position_lerp.y += height;

        if(orbit_enabled) {
            delta_yaw = vectorToYaw(c_trans->getForward());
            MAT44 rot = MAT44::CreateRotationX(delta_pitch) * MAT44::CreateRotationY(delta_yaw);
            collision_position_lerp = target_position_lerp + rot.Forward() * distance;
        }

        h_player = e_target->get<TCompPlayerController>();
    }

    if (is_locked_on) {
        lockedMovement(dt);

        VEC2 delta = input->getMouse().delta;

        // If there is no mouse delta, check if there is a lock on target change with the pad
        if (delta == VEC2::Zero && PlayerInput.getPad().connected) {
          delta = VEC2(PlayerInput[input::EPadButton::RANALOG_X].value, -PlayerInput[input::EPadButton::RANALOG_Y].value);
        }

        if (time_elapsed >= 0.3f && delta.Length() > 0.1f) {

            TCompPlayerController* c_player = h_player;
            
            delta.Normalize();
            delta.y = -delta.y;
            c_player->changeLockedTarget(delta);
            time_elapsed = 0.0f;
        }
    }
    else if (must_recenter && orbit_enabled) {
        must_recenter = false;
        TCompTransform* p_trans = h_trans_target;
        vectorToYawPitch(p_trans->getForward(), &delta_yaw, &delta_pitch);
    }
    else if(orbit_enabled) {
        // Used to detect right/left movement
        TCompPlayerController* c_player = h_player;
        float rot_factor = c_player->getRotFactor();
        float speed = c_player->getSpeed();

        // For orbit around player in horizontal movement
        // (speed / distance) is angle in radians obtained from length of an arc formula -> (Arc length = 2pi*r (angle/360))
        delta_yaw += (speed / distance) * dt * rot_factor;
        updateDeltas(dt);
    }

    float angleLerpFactor = getAngleLerpFactor();
    delta_pitch_lerp = lerpRadians(delta_pitch_lerp, delta_pitch, 8.0f * angleLerpFactor, dt);
    delta_yaw_lerp = lerpRadians(delta_yaw_lerp, delta_yaw, 12.0f * angleLerpFactor, dt);

    // Modify camera settings when locked depending on the distance to the target
    float finalDistance = distance;
    float finalHeight = height;
    TCompTransform* c_trans = h_trans_target;
    TCompTransform* t_locked = h_trans_locked_enemy;

    if (t_locked) {
        float lockedUnitDistance = c_trans->distance(*t_locked) / MAX_LOCKED_DISTANCE;
        // More distance if close to locked
        finalDistance += (1.f - lockedUnitDistance) * 0.65f;
        // More height if close to locked
        finalHeight += (1.f - lockedUnitDistance) * 0.2f;
    }

    // Apply offset depending on the axis restrictions
    VEC3 target_pos = c_trans->getPosition();
    target_pos += getRestrictOffset(target_pos);
    
    // Don't follow if death
    {
        if (target_dead)
            target_pos = lastTargetPosition;
        else
            lastTargetPosition = target_pos;
    }

    target_pos.y += finalHeight;

    float lerp_velocity = getLerpFactor();
    target_position_lerp = dampCubicIn<VEC3>(target_position_lerp, target_pos, lerp_velocity, dt);

    if (!enabled)
        return;

    // Orbit movement
    MAT44 rot = MAT44::CreateRotationX(delta_pitch_lerp) * MAT44::CreateRotationY(delta_yaw_lerp);
    VEC3 position = target_position_lerp + rot.Forward() * finalDistance;

    TCompPlayerController* c_player = h_player;
    bool is_crossing_objects = std::get<bool>(c_player->getVariable("is_crossing_wall"));
    is_crossing_objects |= std::get<bool>(c_player->getVariable("is_crossing_rift"));

    // Keep camera inside scenario
    // In case of 3d follow camera
    if(orbit_enabled) {

        VEC3 rayDir = position - target_pos;
        float dist_to_player = rayDir.Length();
        rayDir.Normalize();

        physx::PxU32 layerMask = CModulePhysics::FilterGroup::Scenario;
        
        if (!is_crossing_objects)
            layerMask |= CModulePhysics::FilterGroup::Interactable;

        std::vector<physx::PxSweepHit> sweepHits;
        physx::PxTransform trans(VEC3_TO_PXVEC3(target_pos));
        float sphere_radius = 0.1f; // Should be equal to camera's z near
        physx::PxSphereGeometry geometry = { sphere_radius };
        bool hasHit = EnginePhysics.sweep(trans, rayDir, dist_to_player, geometry, sweepHits, layerMask, true, false);
        if (hasHit) {
            position = PXVEC3_TO_VEC3(sweepHits[0].position) + PXVEC3_TO_VEC3(sweepHits[0].normal) * sphere_radius;
        }
    }

    collision_position_lerp = damp<VEC3>(collision_position_lerp, position, 20.f, dt);

    // Restrict movement in any axis
    applyAxisRestrictions(collision_position_lerp);
    c_transform->lookAt(collision_position_lerp, target_position_lerp, VEC3::Up);
}

float TCompCameraFollow::getAngleLerpFactor()
{
    float factor = is_locked_on ? 0.5f : 1.f;

    // don't lerp at sprint attack
    CEntity* player = getEntityByName("player");
    TCompFSM* fsm = player->get<TCompFSM>();
    auto state = fsm->getCurrentState();
    assert(fsm);
    if (state && state->name == "SprintAttack") {
        return 0.f;
    }

    return factor;
}

VEC3 TCompCameraFollow::getRestrictOffset(VEC3 target_pos)
{
    VEC3 limits = VEC3(
        restrict_x_axis ? mapToRange(limits_x_axis.x, limits_x_axis.y, 0.f, 1.f, target_pos.x) : 1.f,
        restrict_y_axis ? mapToRange(limits_y_axis.x, limits_y_axis.y, 0.f, 1.f, target_pos.y) : 1.f,
        restrict_z_axis ? mapToRange(limits_z_axis.x, limits_z_axis.y, 0.f, 1.f, target_pos.z) : 1.f
    );

    return restrict_offset * limits;
}

void TCompCameraFollow::applyAxisRestrictions(VEC3& dst)
{
    dst.x = restrict_x_axis ? original_pos.x : dst.x;
    dst.y = restrict_y_axis ? original_pos.y : dst.y;
    dst.z = restrict_z_axis ? original_pos.z : dst.z;
}

void TCompCameraFollow::lockedMovement(float dt)
{
    VEC3 locked_pos;
    VEC3 camera_front;

    assert(h_trans_locked_enemy.isValid());

    CEntity* eLocked = h_trans_locked_enemy.getOwner();

    if (checkUnlockDistance(eLocked, locked_pos, camera_front)) {
        setLockedEntity(CHandle());
        return;
    }
    
    camera_front.Normalize();

    vectorToYawPitch(camera_front, &delta_yaw, &delta_pitch);

    delta_pitch = std::min(delta_pitch, 0.5f);
    delta_pitch = std::max(delta_pitch, -0.2f);

    // update lock target to render something in that pos
    lock_target = locked_pos;
}

float TCompCameraFollow::getLerpFactor()
{
    CEntity* player = getEntityByName("player");
    TCompCollider* c_collider = player->get<TCompCollider>();
    float speed = c_collider->getLinearVelocity().Length();
    speed = std::clamp(speed, 5.0f, 10.0f);
    return speed * lerp_scale;
}

void TCompCameraFollow::setLockedEntity(CHandle handle)
{
    if (handle.isValid()) {
        CEntity* hEntity = handle;
        assert(hEntity);
        h_trans_locked_enemy = hEntity->get<TCompTransform>();
        assert(h_trans_locked_enemy.isValid());
        is_locked_on = true;
    }
    else {

        lock_target = VEC3(-1e8f);

        // delete locking info
        h_trans_locked_enemy = CHandle();
        is_locked_on = false;

        // don't call reset() here, would form a loop
        TCompPlayerController* c_player = h_player;
        c_player->removeLockOn();
    }
}

void TCompCameraFollow::reset(bool spawn_reset)
{
    if (is_locked_on) {
        setLockedEntity(CHandle());
    }
    else {
        must_recenter = true;
    }

    if (spawn_reset)
        target_dead = false;
}

VEC3 TCompCameraFollow::getLockedPos(CHandle h_locked_entity, float& finalHeight)
{
    VEC3 locked_pos;

    CEntity* eLocked = h_locked_entity;
    TCompTransform* t_locked = eLocked->get<TCompTransform>();
    TCompLockTarget* lockTarget = eLocked->get<TCompLockTarget>();

    // Update position to lerp
    if (lockTarget) {
        locked_pos = lockTarget->getPosition();

        float lockHeight = lockTarget->getHeight();

        if (lockHeight != -1.f)
            finalHeight = lockHeight;
    }
    else {
        locked_pos = t_locked->getPosition();
    }

    return locked_pos;
}

bool TCompCameraFollow::checkUnlockDistance(CHandle h_locked_entity)
{
    VEC3 dummy_1, dummy_2;
    return checkUnlockDistance(h_locked_entity, dummy_1, dummy_2);
}

bool TCompCameraFollow::checkUnlockDistance(CHandle h_locked_entity, VEC3& locked_pos, VEC3& camera_front)
{
    CEntity* hEntity = h_locked_entity;
    assert(hEntity);

    // Cast handles to get each TCompTransform
    TCompTransform* t_target = h_trans_target;
    TCompTransform* t_locked = hEntity->get<TCompTransform>();

    float finalHeight = height;
    float lockedUnitDistance = t_target->distance(*t_locked) / MAX_LOCKED_DISTANCE;

    CEntity* eLocked = h_locked_entity;
    locked_pos = getLockedPos(eLocked, finalHeight);

    VEC3 target_pos = t_target->getPosition();
    target_pos.y += finalHeight + finalHeight * lockedUnitDistance;

    camera_front = locked_pos - target_pos;

    return camera_front.Length() > MAX_LOCKED_DISTANCE;
}