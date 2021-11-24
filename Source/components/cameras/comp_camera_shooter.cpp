#include "mcv_platform.h"
#include "engine.h"
#include "components/messages.h"
#include "input/input_module.h"
#include "render/draw_primitives.h"
#include "render/render.h"
#include "components/cameras/comp_camera_shooter.h"
#include "components/common/comp_transform.h"
#include "modules/module_camera_mixer.h"
#include "modules/module_physics.h"

DECL_OBJ_MANAGER("camera_shooter", TCompCameraShooter)

void TCompCameraShooter::onEntityCreated()
{
    input = CEngine::get().getInput(input::PLAYER_1);
    assert(input);

    name = "camera_shooter";

    TMsgCameraCreated msg;
    msg.name = name;
    msg.camera = CHandle(this);
    GameManager->sendMsg(msg);
}

void TCompCameraShooter::load(const json& j, TEntityParseContext& ctx)
{
    target = j.value("target", "");
    distance = j.value("distance", distance);
    offset = loadVEC3(j, "offset");
    
    rotation_sensibility = j.value("rotation_sensibility", rotation_sensibility);
    rotation_sensibility = deg2rad(rotation_sensibility);
    orbit_speed = j.value("orbit_speed", orbit_speed);

}

void TCompCameraShooter::update(float dt)
{
    if (!enabled)
        return;

    // If the transform is not valid, can be either its the first time,
    // or Eon is dead
    if (!h_trans_target.isValid())
    {
        CEntity* e_target = getEntityByName(target);

        if (!e_target)
            return;

        h_trans_target = e_target->get<TCompTransform>();
        TCompTransform* trans = h_trans_target;
        target_position_lerp = trans->getPosition();
        target_position_lerp += offset;
    }

    updateDeltas(dt);

    delta_pitch_lerp.value = dampRadians(delta_pitch_lerp.value, delta_pitch, delta_pitch_lerp.velocity, 0.05f, dt);
    delta_yaw_lerp.value = dampRadians(delta_yaw_lerp.value, delta_yaw, delta_yaw_lerp.velocity, 0.05f, dt);

    TCompTransform* c_transform = get<TCompTransform>();
    TCompTransform* t_target = h_trans_target;

    MAT44 m_player_rot = MAT44::CreateRotationY(delta_yaw_lerp.value);
    QUAT player_rot = QUAT::CreateFromRotationMatrix(m_player_rot);
    t_target->setRotation(player_rot);

    // Local offset
    CTransform t;
    t.setPosition(offset);

    t = t_target->combinedWith(t);
    VEC3 target_pos = t.getPosition();

    target_position_lerp = VEC3::Lerp(target_position_lerp, target_pos, 9.f * dt);

    MAT44 m_camera_rot = MAT44::CreateRotationX(delta_pitch_lerp.value) * MAT44::CreateRotationY(delta_yaw_lerp.value);
    VEC3 cameraFwd = m_camera_rot.Forward();
    cameraFwd = VEC3::Transform(cameraFwd, QUAT::CreateFromAxisAngle(VEC3::Up, deg2rad(25)));
    VEC3 position = target_position_lerp + cameraFwd * distance;
    VEC3 target = position - m_camera_rot.Forward();

    // Keep camera inside scenario
    {
        VEC3 rayDir = position - target_pos;
        float dist_to_player = rayDir.Length();
        rayDir.Normalize();

        physx::PxU32 layerMask = CModulePhysics::FilterGroup::Scenario;

        std::vector<physx::PxSweepHit> sweepHits;
        physx::PxTransform trans(VEC3_TO_PXVEC3(target_pos));
        float sphere_radius = 0.1f; // Should be equal to camera's z near
        physx::PxSphereGeometry geometry = { sphere_radius };
        bool hasHit = EnginePhysics.sweep(trans, rayDir, dist_to_player, geometry, sweepHits, layerMask, true, false);
        if (hasHit) {
            position = PXVEC3_TO_VEC3(sweepHits[0].position) + PXVEC3_TO_VEC3(sweepHits[0].normal) * sphere_radius;
        }
    }
    
    c_transform->lookAt(position, target, VEC3::Up);
}

void TCompCameraShooter::debugInMenu()
{
    ImGui::Checkbox("Enabled", &enabled);
    ImGui::Text("%s", target.c_str());
    ImGui::DragFloat3("Offset", &offset.x, 0.01f, -20.f, 20.f);
    ImGui::DragFloat("Distance", &distance, 0.01f, 0.f, 20.f);
    ImGui::Separator();
    ImGui::DragFloat("Yaw", &delta_yaw, 0.01f, 0.f, (float)M_PI_2);
    ImGui::DragFloat("Pitch", &delta_pitch, 0.01f, 0.f, (float)M_PI_2);
}
