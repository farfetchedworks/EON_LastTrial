#pragma once

#include "components/cameras/comp_gameplay_camera.h"
#include "components/common/comp_base.h"
#include "entity/entity.h"

namespace input {
    class CModule;
}

struct TCompCameraShooter : public IGameplayCamera, TCompBase {

    DECL_SIBLING_ACCESS();

    std::string target;

    CHandle h_trans_target;             // Transform of camera's target
    VEC3 target_position_lerp;
    VEC3 offset;

    float distance              = 0.0f;

    void onEntityCreated();
    void load(const json& j, TEntityParseContext& ctx);
    void debugInMenu();
    void update(float dt);
};