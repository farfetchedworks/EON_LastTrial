#pragma once

namespace input {
    class CModule;
}

struct IGameplayCamera;
class CEntity;

struct IGameplayCamera {

    input::CModule* input       = nullptr;
    std::string name;

    float delta_yaw             = 0.0f;
    float delta_pitch           = 0.0f;

    LerpedValue<float>  delta_yaw_lerp = {};
    LerpedValue<float>  delta_pitch_lerp = {};

    float orbit_speed           = 0.0f;
    float rotation_sensibility  = 0.0f;
    
    bool enabled                = false;
    bool orbit_enabled          = true;

    bool restrict_x_axis        = false;
    bool restrict_y_axis        = false;
    bool restrict_z_axis        = false;

    VEC2 limits_x_axis;
    VEC2 limits_y_axis;
    VEC2 limits_z_axis;

    void deltasFromCamera(const IGameplayCamera& camera);
    void updateDeltas(float dt);
    void clearDeltas();

    void load(const json& j, TEntityParseContext& ctx);
    void enable();
    void disable();

    // is null, get last active camera from Mixer
    static IGameplayCamera* fromCamera(CEntity* camera = nullptr);
    static CHandle getHandle(IGameplayCamera* camera);
};