#pragma once

#include "modules/module.h"
#include "render/deferred_renderer.h"
#include "components/common/comp_compute.h"

class CGPUBuffer;

class CRenderModule : public IModule
{

  public:
    CRenderModule(const std::string& name) : IModule(name){
        load("data/render_module.json");
    }

    void load(const std::string& filename);
    bool start() override;
    void update(float dt) override;
    void stop() override;
    void renderInMenu() override;

    void renderAll();
    void setClearColor(const Color& newColor) { _clearColor = newColor; }
    void setActiveCamera(CHandle hCamera) { _activeCamera = hCamera; }
    void generateFrame();

    bool useDayAmbient() { return _useDayAmbient; };
    void setUseDayAmbient(bool v) { _useDayAmbient = v; };

    CRenderToTexture* getFinalRT() { return rt_final; };
    CHandle getActiveCamera() const { return _activeCamera; }
    void activateMainCamera();

    void startRenderDocCapture();
    void stopRenderDocCapture();
    
    static void updateWorldCtes(float width, float height);
    void updateAllCtesBones();
    static bool RENDER_IMGUI;

  private:
    
    void onFileChanged(const std::string& filename) override;
    void loadPipelines();
    void loadComputeShaders();
    void generateShadowMaps();

    bool _useDayAmbient = false;

    Color _clearColor;
    CHandle _activeCamera;
    CDeferredRenderer deferred_renderer;
    CRenderToTexture* rt_deferred_output = nullptr;
    CRenderToTexture* rt_final = nullptr;
    
    CTexture* _blackHole = nullptr;
    CTexture* _lastOutput = nullptr;
    
    TCompCompute comp_exposure;
    CGPUBuffer* exposure_histogram = nullptr;
    CTexture* exposure_avg_texture = nullptr;

    std::map<std::string, CPipelineState*> my_pipelines;
    void createPipelineFromJson(const std::string& name, const json& jdef);
};