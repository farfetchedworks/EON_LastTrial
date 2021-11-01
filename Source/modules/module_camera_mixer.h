#pragma once

#include "modules/module.h"
#include "entity/entity.h"

struct TMixedCamera
{
    CHandle entity;
    float blendTime = 0.f;
    float weight = 0.f;
    bool abandoned = false;
    const interpolators::IInterpolator* interpolator = nullptr;
};

class CModuleCameraMixer : public IModule
{
private:

public:
    CModuleCameraMixer(const std::string& name) : IModule(name) { }
    bool start() override;
    void stop() override;
    void update(float delta) override;
    void renderInMenu() override;

    void blendCamera(const std::string& cameraName, float blendTime = 0.f, const interpolators::IInterpolator* interpolator = nullptr);
    void setDefaultCamera(CHandle hCamera) { _defaultCamera = hCamera; }
    void setOutputCamera(CHandle hCamera) { _outputCamera = hCamera; }

    const TMixedCamera& getLastCamera();
    const TMixedCamera& getCameraByName(const std::string& name);

private:
    void lerpCameras(CHandle hCamera1, CHandle hCamera2, CHandle hOutput, float ratio) const;

    std::vector<TMixedCamera> _mixedCameras;
    CHandle _defaultCamera;
    CHandle _outputCamera;
    bool _enabled = true;
};