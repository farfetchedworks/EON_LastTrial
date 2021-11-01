#pragma once

#include "components/cameras/comp_gameplay_camera.h"
#include "components/common/comp_base.h"
#include "entity/entity.h"

namespace input {
    class CModule;
}

struct TCompCameraPreview : public TCompBase, IGameplayCamera {

    DECL_SIBLING_ACCESS();

    std::string target;

    float distance = 0.0f;
    float height = 0.0f;
    float pan = 0.0f;

    VEC3 target_position_lerp;

    CHandle h_trans_target;  // Transform of camera's target

    bool must_recenter = false;

    void onEntityCreated();
    void load(const json& j, TEntityParseContext& ctx);
    void update(float dt);
};