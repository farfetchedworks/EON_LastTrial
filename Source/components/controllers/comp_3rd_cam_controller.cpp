#include "mcv_platform.h"
#include "components/common/comp_base.h"
#include "components/common/comp_transform.h"
#include "entity/entity.h"
#include "geometry/curve.h"
#include "input/input_module.h"
#include "engine.h"

struct TComp3rdCameraController : public TCompBase
{
    DECL_SIBLING_ACCESS()

    const CCurve* curve = nullptr;
    float yawSensitivity = 1.f;
    float pitchSensitivity = 1.f;
    CHandle hTarget;
    VEC3 offset = VEC3::Zero;


    void load(const json& j, TEntityParseContext& ctx)
    {
        const std::string& filename = j["curve"];
        curve = Resources.get(filename)->as<CCurve>();
        
        yawSensitivity = j.value("yaw_sensitivity", yawSensitivity);
        pitchSensitivity = j.value("pitch_sensitivity", pitchSensitivity);
        hTarget = getEntityByName(j["target"]);
        offset = loadVEC3(j, "offset");
    }

    void update(float dt)
    {
        CEntity* eTarget = hTarget;
        if (!eTarget)
          return;

        TCompTransform* cTarget = eTarget->get<TCompTransform>();
        if (!cTarget) return;

        TCompTransform* myTransform = get<TCompTransform>();
        if (!myTransform) return;

        readInput(dt);

        const MAT44 tr = MAT44::CreateTranslation(offset);
        const MAT44 rt = MAT44::CreateFromYawPitchRoll(yaw, 0.f, 0.f);
        const MAT44 world = rt * tr * cTarget->asMatrix();

        const VEC3 newPosition = curve->evaluate(ratio, world);
        myTransform->lookAt(newPosition, cTarget->getPosition(), VEC3::Up);
    }

    void readInput(float dt)
    {
        input::CModule* input = CEngine::get().getInput();
        assert(input);

        if (input->getKey(VK_UP).isPressed())
        {
            ratio = std::clamp(ratio + pitchSensitivity * dt, 0.f, 1.f);
        }
        else if (input->getKey(VK_DOWN).isPressed())
        {
            ratio = std::clamp(ratio - pitchSensitivity * dt, 0.f, 1.f);
        }

        if (input->getKey(VK_LEFT).isPressed())
        {
            yaw += yawSensitivity * dt;
        }
        else if (input->getKey(VK_RIGHT).isPressed())
        {
            yaw -= yawSensitivity * dt;
        }
    }

    void debugInMenu()
    {
        if (ImGui::TreeNode("Curve"))
        {
            curve->renderInMenu();
            ImGui::TreePop();
        }
        ImGui::DragFloat("Yaw Sensitivity", &yawSensitivity);
        ImGui::DragFloat("Pitch Sensitivity", &pitchSensitivity);
    }

    void renderDebug()
    {
        CEntity* eTarget = hTarget;
        if (!eTarget)
          return;
        TCompTransform* cTarget = eTarget->get<TCompTransform>();
        if (cTarget)
        {
            const MAT44 tr = MAT44::CreateTranslation(offset);
            const MAT44 rt = MAT44::CreateFromYawPitchRoll(yaw, 0.f, 0.f);
            const MAT44 world = rt * tr * cTarget->asMatrix();

            curve->renderDebug(world, VEC4(1.f, 1.f, 0.f, 1.f));
        }
    }

private:
    MAT44 initialTransform;
    float ratio = 0.f;
    float yaw = 0.f;
};

DECL_OBJ_MANAGER("3rd_camera_controller", TComp3rdCameraController)