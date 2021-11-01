#include "mcv_platform.h"
#include "engine.h"
#include "ui/ui_module.h"
#include "ui/widgets/ui_image.h"
#include "render/draw_primitives.h"
#include "ui/ui_parser.h"
#include "ui/ui_widget.h"
#include "ui/effects/ui_effect_fade.h"

extern CShaderCte<CtesUI> cte_ui;

namespace ui
{
    CModule::CModule(const std::string& name)
        : IModule(name)
    {}

    bool CModule::start()
    {
        _resolution = VEC2(1920.f, 1080.f);
        _camera.setOrthoParams(false, 0.f, _resolution.x, 0.f, _resolution.y, -1.f, 1.f);

        parser::parseFonts("data/ui/fonts.json");
        parser::parseWidgetsFileList("data/ui/widgets.json");

        _pipelineCombinative = Resources.get("ui_combinative.pipeline")->as<CPipelineState>();
        _pipelineAdditive = Resources.get("ui_additive.pipeline")->as<CPipelineState>();
        _pipelineVideoCombinative = Resources.get("ui_video_combinative.pipeline")->as<CPipelineState>();
        _pipelineVideoAdditive = Resources.get("ui_video_additive.pipeline")->as<CPipelineState>();
        _pipelineHUD = Resources.get("ui_hud.pipeline")->as<CPipelineState>();
        _pipelineDebug = Resources.get("debug.pipeline")->as<CPipelineState>();
        _mesh = Resources.get("unit_quad_xy.mesh")->as<CMesh>();

        return true;
    }

    void CModule::setResolution(const VEC2& res)
    {
        _resolution = res;
        _camera.setOrthoParams(false, 0.f, _resolution.x, 0.f, _resolution.y, -1.f, 1.f);
    }

    void CModule::registerWidget(CWidget* widget)
    {
        assert(widget);
        _registeredWidgets[widget->getName()] = widget;
    }

    void CModule::registerAlias(CWidget* widget)
    {
        assert(widget && !widget->getAlias().empty());
        _registeredAlias[widget->getAlias()] = widget;
    }

    void CModule::setWidgetActive(const std::string& name, bool active)
    {
        if (active) {
            activateWidget(name);
        }
        else {
            deactivateWidget(name);
        }
    }

    void CModule::activateWidget(const std::string& name, bool fade_in)
    {
        CWidget* widget = getWidget(name);
        assert(widget);
        if(!widget || widget->isActive()) return;

        if (fade_in && widget->hasEffect("Fade In")) {
            widget->setState(EState::STATE_IN);
        }
        
        _activeWidgets.push_back(widget);
        widget->setActive(true);
    }

    void CModule::deactivateWidget(const std::string& name, bool fade_out)
    {
        CWidget* widget = getWidget(name);
        assert(widget);
        if (!widget || !widget->isActive()) return;

        if (fade_out && widget->hasEffect("Fade Out")) {
            widget->setState(EState::STATE_OUT);
        }
        else {
            widget->setState(EState::STATE_NONE);
            widget->setActive(false);
            _activeWidgets.erase(std::remove(_activeWidgets.begin(), _activeWidgets.end(), widget));
        }
    }

    CWidget* CModule::getWidget(const std::string& name)
    {
        auto it = _registeredWidgets.find(name);
        if(it != _registeredWidgets.end())
            return it->second;

        it = _registeredAlias.find(name);
        if (it != _registeredAlias.end())
            return it->second;

        return nullptr;
    }

    CWidget* CModule::getWidgetFrom(const std::string& parent_widget, const std::string& name)
    {
        auto it = _registeredWidgets.find(parent_widget);
        if (it != _registeredWidgets.end()) {
            for (auto c : it->second->getChildren()) {
                if (c->getName() == name)
                    return c;
            }
        }
        return nullptr;
    }

    void CModule::registerFont(TFontParams& fontParams)
    {
        _registeredFonts[fontParams.name] = fontParams;
    }

    const TFontParams* CModule::getFont(const std::string& name) const
    {
        auto it = _registeredFonts.find(name);
        return it != _registeredFonts.end() ? &it->second : nullptr;
    }

    void CModule::update(float delta)
    {
        // Remove widgets to clear from active 
        auto eraseIt = std::remove_if(begin(_activeWidgets), end(_activeWidgets), [](CWidget * w) {
            bool to_clear = (w->getState() == EState::STATE_CLEAR);
            if (to_clear) {
                w->setState(EState::STATE_NONE);
                w->setActive(false);
            }
            return to_clear;
        });

        _activeWidgets.erase(eraseIt, end(_activeWidgets));

        for (CWidget* widget : _activeWidgets)
        {
            widget->updateRecursive(delta);
        }
    }

    void CModule::renderUI()
    {
        activateCamera(_camera);

        for (CWidget* widget : _activeWidgets)
        {
            widget->renderRecursive();
        }

        if (!_showDebug)
        {
            return;
        }

        for (auto& widget : _activeWidgets)
        {
            widget->renderDebugRecursive();
        }
    }

    void CModule::renderUIDebug()
    {
        if (!_showDebug)
        {
            return;
        }

        activateCamera(_camera);

        _pipelineDebug->activate();

        for (auto& widget : _activeWidgets)
        {
            widget->renderDebugRecursive();
        }
    }

    void CModule::renderVideo(const MAT44& worldObject, const TVideoParams& params, const VEC2& contentSize)
    {
        const MAT44 sz = MAT44::CreateScale(contentSize.x, contentSize.y, 1.f);
        const MAT44 world = sz * worldObject;

        // pipeline
        params.additive ? _pipelineVideoAdditive->activate() : _pipelineVideoCombinative->activate();

        // texture
        params.video->getTexture()->activate(TS_ALBEDO);

        // shader constants
        activateObject(world);

        cte_ui.ui_min_uv = VEC2::Zero;
        cte_ui.ui_max_uv = VEC2::One;
        cte_ui.ui_fade_in = params.time_normalized;
        cte_ui.updateFromCPU();

        // mesh
        _mesh->activate();
        _mesh->render();
    }

    void CModule::renderBitmap(const MAT44& worldObject, const TImageParams& params, const VEC2& contentSize)
    {
        const MAT44 sz = MAT44::CreateScale(contentSize.x, contentSize.y, 1.f);
        const MAT44 world = sz * worldObject;

        // pipeline
        if (params.hud)
        {
            _pipelineHUD->activate();
        }
        else {
            params.additive ? _pipelineAdditive->activate() : _pipelineCombinative->activate();
        }

        // texture
        params.texture->activate(TS_ALBEDO);

        if(params.cut_texture)
            params.cut_texture->activate(TS_NORMAL);

        // shader constants
        activateObject(world, params.color);

        cte_ui.ui_min_uv = params.minUV;
        cte_ui.ui_max_uv = params.maxUV;
        cte_ui.ui_alpha_ratio = params.alpha_cut;
        cte_ui.ui_fade_in = params.time_normalized;
        cte_ui.updateFromCPU();

        // mesh
        _mesh->activate();
        _mesh->render();
    }

    void CModule::renderText(const MAT44& world, const TTextParams& params, const VEC2& contentSize)
    {
        const TFontParams* font = params.font;
        assert(font);
        if(!font) return;

        const VEC2 characterSize = VEC2(static_cast<float>(params.size));

        // alignment
        const float textWidth = params.text.size() * characterSize.x;
        const float textHeight = characterSize.y;

        VEC3 alignOffset = VEC3::Zero;
        if (params.halign == EHAlign::Center)        alignOffset.x = (contentSize.x - textWidth) * 0.5f;
        else if (params.halign == EHAlign::Right)    alignOffset.x = contentSize.x - textWidth;

        if (params.valign == EVAlign::Center)       alignOffset.y = (contentSize.y - textHeight) * 0.5f;
        else if (params.valign == EVAlign::Bottom)  alignOffset.y = contentSize.y - textHeight;

        MAT44 characterWorld = MAT44::CreateTranslation(alignOffset) * world;
        const MAT44 tr = MAT44::CreateTranslation(VEC3(characterSize.x, 0.f, 0.f));

        // characters
        TImageParams characterParams;
        characterParams.texture = font->texture;
        characterParams.color = params.color;

        const char firstCharacter = ' ';
        const int numRows = font->numRows;
        const int numCols = font->numCols;
        const VEC2 cellSize(1.f / static_cast<float>(numCols), 1.f / static_cast<float>(numRows));

        for (auto character : params.text)
        {
            const int idx = character - firstCharacter;
            const int row = idx / numCols;
            const int col = idx % numCols;

            characterParams.minUV = VEC2(col * cellSize.x, row * cellSize.y);
            characterParams.maxUV = characterParams.minUV + cellSize;

            renderBitmap(characterWorld, characterParams, characterSize);

            characterWorld = tr * characterWorld;
        }
    }

    void CModule::renderInMenu()
    {
        if (ImGui::TreeNode("UI"))
        {
            ImGui::Checkbox("Render Debug", &_showDebug);

            if (ImGui::TreeNode("Fonts"))
            {
                for (auto& [name, font] : _registeredFonts)
                {
                    if (ImGui::TreeNode(name.c_str()))
                    {
                        font.renderInMenu();
                        ImGui::TreePop();
                    }
                }
                ImGui::TreePop();
            }

            const auto addWidget = [this](CWidget* widget)
            {
                const std::string& name = widget->getName();
                const std::string dbgname = std::string(widget->getType()) + " - " + name.data();

                if (ImGui::TreeNode(dbgname.c_str()))
                {
                    bool isActive = widget->isActive();
                    if (ImGui::Checkbox("Active", &isActive))
                    {
                        isActive ? activateWidget(name) : deactivateWidget(name);
                    }

                    widget->renderInMenuRecursive();
                    ImGui::TreePop();
                }
            };

            if (ImGui::TreeNode("Widgets"))
            {
                for (auto& [name, widget] : _registeredWidgets)
                {
                    addWidget(widget);
                }
                ImGui::TreePop();
            }

            if (ImGui::TreeNode("Active Widgets"))
            {
                for (auto& widget : _activeWidgets)
                {
                    addWidget(widget);
                }
                ImGui::TreePop();
            }

            ImGui::TreePop();
        }
    }
}
