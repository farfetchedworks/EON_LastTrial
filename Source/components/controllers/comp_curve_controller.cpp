#include "mcv_platform.h"

#include "comp_curve_controller.h"

#include "components/common/comp_base.h"
#include "components/common/comp_transform.h"
#include "entity/entity.h"
#include "geometry/curve.h"

DECL_OBJ_MANAGER("curve_controller", TCompCurveController)

void TCompCurveController::load(const json& j, TEntityParseContext& ctx)
{
    const std::string& filename = j["curve"];
    curve = Resources.get(filename)->as<CCurve>();
    active = j.value("active", active);
    speed = j.value("speed", speed);
}

void TCompCurveController::onEntityCreated()
{
    TCompTransform* c_target = get<TCompTransform>();
    if (!c_target) return;

    initialTransform = c_target->asMatrix();
}

void TCompCurveController::update(float dt)
{
    TCompTransform* c_target = get<TCompTransform>();
    if (!c_target) return;

    VEC3 final_target;
    if (target_entity_transform.isValid()) {
        TCompTransform* c_target_entity = target_entity_transform;
        final_target = c_target_entity->getPosition();
    } else {
        final_target = target;
    }

    if (!active) return;

    ratio += dt * speed;

    if (curve_lerped) {
        ratio_curve_lerped += dt;

        if (ratio_curve_lerped >= curve_lerped_seconds) {
            curve = curve_lerped;
            ratio = 0.0f;
            speed = curve_lerped_speed;
            curve_lerped = nullptr;
        }
    }

    if (lerping_target) {
        ratio_target_lerped += dt;

        final_target = VEC3::Lerp(target, target_lerped, ratio_target_lerped / target_lerped_seconds);

        if (ratio_target_lerped >= target_lerped_seconds) {
            target = target_lerped;
            lerping_target = false;
            target_entity_transform = CHandle();
        }
    }

    VEC3 newPosition = curve->evaluate(ratio, initialTransform);

    if (curve_lerped) {
        VEC3 targetPosition = curve_lerped->evaluate(0, initialTransform);
        newPosition = VEC3::Lerp(newPosition, targetPosition, ratio_curve_lerped / curve_lerped_seconds);
    }

    c_target->lookAt(newPosition, final_target, VEC3(0, 1, 0));
}

void TCompCurveController::setRatio(float newRatio)
{
    ratio = newRatio;
}

void TCompCurveController::setCurve(const std::string& filename)
{
    auto new_curve = Resources.get(filename)->as<CCurve>();

    if (!new_curve) {
        dbg("Could not change to curve %s in curve controller", filename.c_str());
        return;
    }

    curve = new_curve;
    ratio = 0.0f;
}

void TCompCurveController::setCurveLerped(const std::string& filename, float speed, float seconds)
{
    if (seconds == 0.0f) {
        setCurve(filename);
        return;
    }

    auto new_curve = Resources.get(filename)->as<CCurve>();

    if (!new_curve) {
        dbg("Could not lerp to curve %s in curve controller", filename.c_str());
        return;
    }

    ratio_curve_lerped = 0.0f;
    curve_lerped = new_curve;
    curve_lerped_seconds = seconds;
    curve_lerped_speed = speed;
}

void TCompCurveController::setTarget(const VEC3& new_target)
{
    target = new_target;
    target_entity_transform = CHandle();
}

void TCompCurveController::setTargetEntity(const std::string& target_entity)
{
    CEntity* entity = getEntityByName(target_entity);
    assert(entity);
    if (!entity) {
        return;
    }

    target_entity_transform = entity->get<TCompTransform>();
}

void TCompCurveController::setTargetLerped(const VEC3& new_target, float seconds)
{
    if (seconds == 0.0f) {
        setTarget(new_target);
    }

    ratio_target_lerped = 0.0f;
    target_lerped = new_target;
    target_lerped_seconds = seconds;
    lerping_target = true;
}

void TCompCurveController::setSpeed(float new_speed)
{
    speed = new_speed;
}

void TCompCurveController::setActive(bool value)
{
    active = value;
}

void TCompCurveController::debugInMenu()
{
    if (ImGui::TreeNode("Curve"))
    {
        curve->renderInMenu();
        ImGui::TreePop();
    }
    ImGui::Checkbox("Active", &active);
    ImGui::DragFloat("Speed", &speed, 0.1f, 0.0f, 4.0f);
}

void TCompCurveController::renderDebug()
{
#ifdef _DEBUG
    TCompTransform* c_target = get<TCompTransform>();
    if (c_target)
    {
        curve->renderDebug(initialTransform, VEC4(1.f, 1.f, 0.f, 1.f));
    }
#endif
}


