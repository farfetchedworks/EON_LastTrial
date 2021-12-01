#include "mcv_platform.h"

#include "comp_focus_controller.h"

#include "components/common/comp_transform.h"
#include "components/postfx/comp_focus.h"
#include "entity/entity.h"

DECL_OBJ_MANAGER("focus_controller", TCompFocusController)

void TCompFocusController::load(const json& j, TEntityParseContext& ctx)
{

}

void TCompFocusController::onEntityCreated()
{

}

void TCompFocusController::update(float dt)
{
    if (!_enabled || !_target_entity.isValid()) {
        if (_intensity > 0.0f) {
            TCompFocus* c_focus = get<TCompFocus>();
            CShaderCte<CtesFocus>& ctes_focus = c_focus->getCtesFocus();

            _intensity -= dt;
            _intensity = clampf(_intensity, 0.0f, 1.0f);
            ctes_focus.focus_intensity = _intensity;
            ctes_focus.updateFromCPU();

            if (_intensity == 0.0f) {
                TCompFocus* c_focus = get<TCompFocus>();
                c_focus->disable();
            }
        }

        return;
    }

    CEntity* e_camera = getEntity();
    CEntity* e_target = _target_entity;

    TCompFocus* c_focus = get<TCompFocus>();
    CShaderCte<CtesFocus>& ctes_focus = c_focus->getCtesFocus();

    float distance = VEC3::Distance(e_target->getPosition(), e_camera->getPosition());
    ctes_focus.focus_z_center_in_focus = clampf(distance, 0.0f, 100000.0f);

    _intensity += dt;
    _intensity = clampf(_intensity, 0.0f, 1.0f);

    ctes_focus.focus_intensity = _intensity;
    ctes_focus.updateFromCPU();
}

void TCompFocusController::debugInMenu()
{

}

void TCompFocusController::renderDebug()
{
#ifdef _DEBUG

#endif
}

void TCompFocusController::enable(CHandle target_entity, float transition_distance)
{
    _enabled = true;
    _target_entity = target_entity;

    TCompFocus* c_focus = get<TCompFocus>();
    c_focus->enable();

    CShaderCte<CtesFocus>& ctes_focus = c_focus->getCtesFocus();
    ctes_focus.focus_transition_distance = transition_distance;
}

void TCompFocusController::disable()
{
    _enabled = false;
    _target_entity = CHandle();
}
