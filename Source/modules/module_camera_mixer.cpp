#include "mcv_platform.h"
#include "modules/module_camera_mixer.h"
#include "engine.h"
#include "render/render_module.h"
#include "components/common/comp_camera.h"
#include "components/common/comp_transform.h"

bool CModuleCameraMixer::start()
{
    return true;
}

void CModuleCameraMixer::stop()
{

}

void CModuleCameraMixer::update(float delta)
{
    if (!_enabled)
    {
        EngineRender.setActiveCamera(_defaultCamera);
        return;
    }

    bool fullWeight = false;
    for (auto it = rbegin(_mixedCameras); it != rend(_mixedCameras); ++it)
    {
        TMixedCamera& mc = *it;

        if (mc.weight < 1.f)
        {
            mc.weight = std::clamp(mc.weight + (delta / mc.blendTime), 0.f, 1.f);
        }

        mc.abandoned = fullWeight;

        fullWeight = fullWeight || mc.weight >= 1.f;
    }

    // remove abandoned cameras
    auto eraseIt = std::remove_if(begin(_mixedCameras), end(_mixedCameras), [](const TMixedCamera& mc){return mc.abandoned;});
    _mixedCameras.erase(eraseIt, end(_mixedCameras));

    for (TMixedCamera& mc : _mixedCameras)
    {
        const float weight = mc.interpolator ? mc.interpolator->blend(0.f, 1.f, mc.weight) : mc.weight;
        lerpCameras(_outputCamera, mc.entity, _outputCamera, weight);
    }

    // apply results
    EngineRender.setActiveCamera(_outputCamera);
}

void CModuleCameraMixer::blendCamera(const std::string& cameraName, float blendTime, const interpolators::IInterpolator* interpolator)
{
    CHandle hCamera = getEntityByName(cameraName);

    if (!_mixedCameras.empty() && _mixedCameras.back().entity == hCamera)
    {
        TMixedCamera& mc = _mixedCameras.back();
        if (mc.weight == 0.f)
        {
            mc.blendTime = blendTime;
        }
    }
    else
    {
        TMixedCamera mc;
        mc.entity = hCamera;
        mc.blendTime = blendTime;
        mc.weight = mc.blendTime > 0.f ? 0.f : 1.f;
        mc.interpolator = interpolator;

        _mixedCameras.push_back(mc);
    }
}

void CModuleCameraMixer::lerpCameras(CHandle hCamera1, CHandle hCamera2, CHandle hOutput, float ratio) const
{
    CEntity* eCamera1 = hCamera1;
    CEntity* eCamera2 = hCamera2;
    CEntity* eOutput = hOutput;
    assert(eCamera1 && eCamera2 && eOutput);
    TCompTransform* camera1 = eCamera1->get<TCompTransform>();
    TCompTransform* camera2 = eCamera2->get<TCompTransform>();
    TCompTransform* transformOutput = eOutput->get<TCompTransform>();
    assert(camera1 && camera2);

    VEC3 position = VEC3::Lerp(camera1->getPosition(), camera2->getPosition(), ratio);
    VEC3 direction = VEC3::Lerp(camera1->getForward(), camera2->getForward(), ratio);

    {
        TCompCamera* camera1 = eCamera1->get<TCompCamera>();
        TCompCamera* camera2 = eCamera2->get<TCompCamera>();
        TCompCamera* cameraOutput = eOutput->get<TCompCamera>();
        cameraOutput->setFovVerticalRadians(lerp<float>(camera1->getFov(), camera2->getFov(), ratio));
    }

    transformOutput->lookAt(position, position + direction, VEC3::Up);
}

const TMixedCamera& CModuleCameraMixer::getLastCamera()
{
    if(_mixedCameras.size() > 1)
        return _mixedCameras[_mixedCameras.size() - 2]; 
    else
    {
        assert(_mixedCameras.size() == 1);
        return _mixedCameras[0];
    }
}

const TMixedCamera& CModuleCameraMixer::getCameraByName(const std::string& name)
{
    CHandle camera = getEntityByName(name);
    assert(camera.isValid());

    for (TMixedCamera& mc : _mixedCameras)
    {
        assert(mc.entity.isValid());
        CEntity* e = mc.entity;

        if (CHandle(mc.entity) == CHandle(camera))
            return mc;
    }

    assert(_mixedCameras.size() >= 1);
    return _mixedCameras[0];
}

void CModuleCameraMixer::renderInMenu()
{
    const auto getCameraName = [](CHandle h)
    {
        CEntity* e = h;
        return e ? e->getName() : "...";
    };

    if (ImGui::TreeNode("Camera Mixer"))
    {
        if (ImGui::Button(_enabled ? "Disable" : "Enable"))
        {
            _enabled = !_enabled;
        }

        if(ImGui::TreeNode("Cameras"))
        {
            for (auto& mc : _mixedCameras)
            {
                ImGui::ProgressBar(mc.weight, ImVec2(-1, 0), getCameraName(mc.entity));
            }
            ImGui::TreePop();
        }

        ImGui::TreePop();
    }

    interpolators::renderInterpolators();
}
