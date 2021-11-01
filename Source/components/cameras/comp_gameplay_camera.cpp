#include "mcv_platform.h"
#include "engine.h"
#include "input/input_module.h"
#include "render/render_module.h"
#include "components/common/comp_transform.h"
#include "components/cameras/comp_gameplay_camera.h"
#include "components/cameras/comp_camera_follow.h"
#include "components/cameras/comp_camera_shooter.h"
#include "components/cameras/comp_camera_flyover.h"
#include "modules/module_camera_mixer.h"

void IGameplayCamera::load(const json& j, TEntityParseContext& ctx)
{
    orbit_enabled = j.value("orbit_enabled", orbit_enabled);

    restrict_x_axis = j.value("restrict_x_axis", restrict_x_axis);
    restrict_y_axis = j.value("restrict_y_axis", restrict_y_axis);
    restrict_z_axis = j.value("restrict_z_axis", restrict_z_axis);

    limits_x_axis = loadVEC2(j, "limits_x_axis");
    limits_y_axis = loadVEC2(j, "limits_y_axis");
    limits_z_axis = loadVEC2(j, "limits_z_axis");
}

void IGameplayCamera::deltasFromCamera(const IGameplayCamera& camera)
{
    if (!orbit_enabled)
        return;

    delta_yaw = camera.delta_yaw;
    delta_pitch = camera.delta_pitch;
    delta_yaw_lerp = camera.delta_yaw_lerp;
    delta_pitch_lerp = camera.delta_pitch_lerp;
}

void IGameplayCamera::updateDeltas(float dt)
{
    if (debugging && !isPressed(VK_LCONTROL))
        return; 

    // Gamepad queries
    if (input->getPad().connected && input->getButton("move_camera").isPressed())
    {
        float x_now = input->getPadButton(input::EPadButton::RANALOG_X).value;
        float y_now = input->getPadButton(input::EPadButton::RANALOG_Y).value;

        x_now *= orbit_speed * 0.012f;
        y_now *= orbit_speed * 0.012f;

        delta_yaw -= x_now * rotation_sensibility;
        delta_pitch -= y_now * rotation_sensibility;
    }

    // Mouse queries
    {
        // ImVec2 mdelta = ImGui::GetIO().MouseDelta;
        VEC2 delta = input->getMouse().delta;

        delta *= orbit_speed;
        delta.x = clampf(delta.x, -150, 150);

        delta_yaw -= delta.x * rotation_sensibility;
        delta_pitch += delta.y * rotation_sensibility;
    }

    delta_yaw = clampRotation(delta_yaw);
    delta_pitch = clampRotation(delta_pitch);

    float max_offset = 0.5f;

    if (delta_pitch >= M_PI_2 - max_offset && delta_pitch < M_PI) {
        delta_pitch = (float)M_PI_2 - 0.001f - max_offset;
    }

    if (delta_pitch > M_PI && delta_pitch <= 3.0f * M_PI_2 + max_offset) {
        delta_pitch = 3.0f * (float)M_PI_2 + 0.001f + max_offset;
    }
}

void IGameplayCamera::enable()
{
    enabled = true;
}

void IGameplayCamera::disable()
{
    enabled = false;
}

IGameplayCamera* IGameplayCamera::fromCamera(CEntity* camera)
{
    CEntity* e_camera = camera;

    if (!e_camera) {
        const TMixedCamera& last_camera = CameraMixer.getLastCamera();
        e_camera = last_camera.entity;
    }
    
    std::string cameraName = e_camera->getName();

    if (cameraName == "camera_shooter") {
        TCompCameraShooter* camera = e_camera->get<TCompCameraShooter>();
        return camera;
    }
    else if (includes(cameraName, "camera_follow") || includes(cameraName, "camera_interact")) {
        TCompCameraFollow* camera = e_camera->get<TCompCameraFollow>();
        return camera;
    }
    else if (cameraName == "camera_flyover") {
        TCompCameraFlyover* camera = e_camera->get<TCompCameraFlyover>();
        return camera;
    }

    // fatal("Camera type %s not supported\n", cameraName.c_str());
    return nullptr;
}

CHandle IGameplayCamera::getHandle(IGameplayCamera* camera)
{
    CHandle h_camera = getEntityByName(camera->name);

    if (h_camera.isValid()) {

        return h_camera;
    }
    else {
        fatal("Invalid gameplay camera with name %s\n", camera->name.c_str());
        return CHandle();
    }
}