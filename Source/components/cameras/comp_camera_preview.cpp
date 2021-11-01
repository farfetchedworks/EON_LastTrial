#include "mcv_platform.h"
#include "engine.h"
#include "components/messages.h"
#include "components/cameras/comp_camera_preview.h"
#include "components/common/comp_transform.h"
#include "input/input_module.h"
#include "render/draw_primitives.h"
#include "modules/module_physics.h"

DECL_OBJ_MANAGER("camera_preview", TCompCameraPreview)

void TCompCameraPreview::onEntityCreated()
{
    input = CEngine::get().getInput(input::PLAYER_1);
    assert(input);
    name = "camera_preview";
    enabled = true;

    TMsgCameraCreated msg;
    msg.name = name;
    msg.camera = CHandle(this);
    msg.active = enabled;
    GameManager->sendMsg(msg);

    // mod for this camera
    delta_yaw = 3.14f;
}

void TCompCameraPreview::load(const json& j, TEntityParseContext& ctx)
{
    target = j.value("target", "");
    height = j.value("height", height);
    distance = j.value("distance", distance);

    rotation_sensibility = j.value("rotation_sensibility", rotation_sensibility);
    rotation_sensibility = deg2rad(rotation_sensibility);

    orbit_speed = j.value("orbit_speed", orbit_speed);
}

void TCompCameraPreview::update(float dt)
{
    if (!isWndFocused() || !enabled)
        return;

    float delta = Time.delta_unscaled;

    TCompTransform* c_transform = get<TCompTransform>();

    if (!h_trans_target.isValid())
    {
        CEntity* e_target = getEntityByName(target);
          
        if (!e_target)
            return;

        h_trans_target = e_target->get<TCompTransform>();
        TCompTransform* c_trans = h_trans_target;
        target_position_lerp = c_trans->getPosition();
        target_position_lerp.y += height;
    }

    if (must_recenter) {
        must_recenter = false;
        TCompTransform* p_trans = h_trans_target;
        vectorToYawPitch(p_trans->getForward(), &delta_yaw, &delta_pitch);
    }
    else {
        updateDeltas(delta);
    }

    delta_pitch_lerp = lerpRadians(delta_pitch_lerp, delta_pitch, 14.0f, delta);
    delta_yaw_lerp = lerpRadians(delta_yaw_lerp, delta_yaw, 14.0f, delta);

    static float target_distance = distance;

    // Exit game
    if (PlayerInput[VK_ESCAPE].getsPressed()) {
        CApplication::get().exit();
    }

    if (PlayerInput['W'].isPressed())
        height += 0.01f;
    else if (PlayerInput['S'].isPressed())
        height -= 0.01f;
    else if (PlayerInput['A'].isPressed())
        pan -= 0.01f;
    else if (PlayerInput['D'].isPressed())
        pan += 0.01f;

    // Player position
    TCompTransform* c_trans = h_trans_target;
    VEC3 target_pos = c_trans->getPosition();
    target_pos.x += pan;
    target_pos.y += height;

    target_position_lerp = VEC3::Lerp(target_position_lerp, target_pos, 6.f * delta);

    // Orbit movement

    if (ImGui::GetIO().MouseWheel != 0.f && !ImGui::GetIO().WantCaptureMouse)
    {
        target_distance -= ImGui::GetIO().MouseWheel * 0.2f;
    }

    distance = damp(distance, target_distance, 4.f, delta);

    MAT44 rot = MAT44::CreateRotationX(delta_pitch_lerp) * MAT44::CreateRotationY(delta_yaw_lerp);
    VEC3 position = target_position_lerp + rot.Forward() * distance;
    c_transform->lookAt(position, target_position_lerp, VEC3::Up);
}
