#pragma once

#include "modules/module.h"
#include "ui/ui.fwd.h"

namespace ui
{
    class CModule : public IModule
    {
    public:
        CModule(const std::string& name);

        bool start() override;
        void update(float delta) override;
        void renderUI() override;
        void renderUIDebug() override;
        void renderInMenu() override;
       
        void registerWidget(CWidget* widget);
        void registerAlias(CWidget* widget);
        void setWidgetActive(const std::string& name, bool active);
        CWidget* activateWidget(const std::string& name, bool fade_in = true);
        void deactivateWidget(const std::string& name, bool fade_out = true);
        void fadeWidget(const std::string& name, float time);
        CWidget* getWidget(const std::string& name);
        CWidget* getWidgetFrom(const std::string& parent_widget, const std::string& name);

        void registerFont(TFontParams& fontParams);
        const TFontParams* getFont(const std::string& name) const;

        void renderVideo(const MAT44& worldObject, const TVideoParams& params, const VEC2& contentSize);
        void renderBitmap(const MAT44& world, const TImageParams& params, const VEC2& contentSize);
        void renderText(const MAT44& world, const TTextParams& params, const VEC2& contentSize);
        VEC2& getResolution() { return _resolution; }
        float getWidth() { return _resolution.x; }
        float getHeight() { return _resolution.y; }

        void setResolution(const VEC2& res);
        void fadeOut(float time);

    private:
        VEC2 _resolution;
        CCamera _camera;

        struct TFadeWidget {
            CWidget* widget = nullptr;
            float timer = 0.f;
        };

        // test
        CImage* background = nullptr;

        std::map<std::string_view, CWidget*> _registeredWidgets;
        std::map<std::string_view, CWidget*> _registeredAlias;
        std::vector<CWidget*> _activeWidgets;
        std::vector<TFadeWidget> _fadingWidgets;
        std::map<std::string, TFontParams> _registeredFonts;

        const CPipelineState* _pipelineCombinative = nullptr;
        const CPipelineState* _pipelineAdditive = nullptr;

        const CPipelineState* _pipelineVideoCombinative = nullptr;
        const CPipelineState* _pipelineVideoAdditive = nullptr;

        const CPipelineState* _pipelineHUD = nullptr;
        const CPipelineState* _pipelineDebug = nullptr;

        const CMesh* _mesh = nullptr;
        bool _showDebug = false;
    };
}
