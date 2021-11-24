#include "mcv_platform.h"
#include "components/common/comp_base.h"
#include "components/common/comp_transform.h"
#include "entity/entity.h"
#include "geometry/curve.h"

struct TCompCurveController : public TCompBase
{
    DECL_SIBLING_ACCESS()

    const CCurve* curve = nullptr;
    const CCurve* curve_lerped = nullptr;
    float ratio = 0.f;
    float ratio_curve_lerped = 0.f;
    float curve_lerped_seconds = 0.0f;
    float curve_lerped_speed = 0.0f;

    VEC3 target;
    VEC3 target_lerped;

    // In case we set an antity as target
    CHandle target_entity_transform;

    float ratio_target_lerped = 0.0f;
    float target_lerped_seconds = 0.0f;
    bool lerping_target = false;

    bool active = false;
    float speed = 0.f;

    void load(const json& j, TEntityParseContext& ctx);
    void onEntityCreated();
    void update(float dt);

    const CCurve* getCurve() { return curve; }

    void setRatio(float newRatio);
    void setTarget(const VEC3& new_target);
    void setTargetEntity(const std::string& target_entity);
    void setTargetLerped(const VEC3& new_target, float seconds);
    void setSpeed(float new_speed);
    void setActive(bool value);
    void setCurve(const std::string& filename);
    void setCurveLerped(const std::string& filename, float speed, float seconds);

    void debugInMenu();
    void renderDebug();

private:
    MAT44 initialTransform;
};
