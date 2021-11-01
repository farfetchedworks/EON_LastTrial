#pragma once

#include "components/cameras/comp_gameplay_camera.h"
#include "components/common/comp_base.h"
#include "entity/entity.h"

namespace input {
    class CModule;
}

struct TCompCameraFlyover : public IGameplayCamera, TCompBase {

    DECL_SIBLING_ACCESS();

    float speed_factor = 1.0f;
    float rotation_sensibility = 0.5f;
    VEC3  ispeed = VEC3::Zero;      // inertial speed

    void onEntityCreated();
    void load(const json& j, TEntityParseContext& ctx);
    void update(float dt);
};