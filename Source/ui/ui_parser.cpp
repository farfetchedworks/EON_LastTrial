#include "mcv_platform.h"
#include "ui/ui_parser.h"
#include "ui/ui_module.h"
#include "ui/ui_widget.h"
#include "ui/widgets/ui_image.h"
#include "ui/widgets/ui_video.h"
#include "ui/widgets/ui_text.h"
#include "ui/widgets/ui_bar.h"
#include "ui/widgets/ui_button.h"
#include "ui/effects/ui_effect_anim_uvs.h"
#include "ui/effects/ui_effect_fade.h"
#include "engine.h"

namespace ui
{
    namespace parser
    {
        void parseWidgetsFileList(const std::string& filename)
        {
            const json jData = loadJson(filename);
            for (const json& jFileEntry : jData)
            {
                parseWidgetsFile(jFileEntry);
            }
        }

        void parseWidgetsFile(const std::string& filename)
        {
            const json jData = loadJson(filename);
            CWidget* widget = parseWidget(jData);

            widget->updateLocalTransform();

            CEngine::get().getUI().registerWidget(widget);
        }

        void parseFonts(const std::string& filename)
        {
            const json jData = loadJson(filename);

            for (auto& jEntry : jData)
            {
                TFontParams params;
                parseFontParams(params, jEntry);

                CEngine::get().getUI().registerFont(params);
            }
        }

        CWidget* parseWidget(const json& jData)
        {
            CWidget* widget = nullptr;

            const std::string& type = jData.value("type", "widget");
            if (type == "image")
            {
                widget = parseImage(jData);
            }
            else if (type == "video")
            {
                widget = parseVideo(jData);
            }
            else if (type == "text")
            {
                widget = parseText(jData);
            }
            else if (type == "progress_bar")
            {
                widget = parseProgressBar(jData);
            }
            else if (type == "button")
            {
                widget = parseButton(jData);
            }
            else
            {
                widget = new CWidget();
            }

            parseBaseParams(*widget, jData);

            if (jData.count("effects") > 0)
            {
                for (auto& jEffect : jData["effects"])
                {
                    CEffect* effect = parseEffect(jEffect);
                    if (effect)
                    {
                        widget->addEffect(effect);
                    }
                }
            }

            if (jData.count("children") > 0)
            {
                for (auto& jChild : jData["children"])
                {
                    CWidget* child = parseWidget(jChild);
                    widget->addChild(child);
                }
            }

            if (!widget->getAlias().empty())
            {
                CEngine::get().getUI().registerAlias(widget);
            }

            return widget;
        }

        CWidget* parseVideo(const json& jData)
        {
            CVideo* video = new CVideo();
            parseVideoParams(video->videoParams, jData);
            return video;
        }

        CWidget* parseImage(const json& jData)
        {
            CImage* image = new CImage();
            parseImageParams(image->imageParams, jData);
            return image;
        }

        CWidget* parseText(const json& jData)
        {
            CText* text = new CText();
            parseTextParams(text->textParams, jData);
            return text;
        }

        CWidget* parseProgressBar(const json& jData)
        {
            CProgressBar* bar = new CProgressBar();
            parseImageParams(bar->imageParams, jData);
            return bar;
        }

        CWidget* parseButton(const json& jData)
        {
            CButton* button = new CButton();
            parseImageParams(button->imageParams, jData);
            parseTextParams(button->textParams, jData);

            for (auto& jState : jData["states"])
            {
                CButton::TStateParams stateParams;
                stateParams.imageParams = button->imageParams;
                stateParams.textParams = button->textParams;

                parseImageParams(stateParams.imageParams, jState);
                parseTextParams(stateParams.textParams, jState);

                button->addState(jState["name"], stateParams);
            }

            return button;
        }

        CEffect* parseEffect(const json& jData)
        {
            CEffect* effect = nullptr;

            const std::string& type = jData.value("type", "");
            if (type == "animate_uv")
            {
                effect = parseAnimateUVs(jData);
            }
            else if (type == "fade_in")
            {
                effect = parseFadeIn(jData);
            }
            else if (type == "fade_out")
            {
                effect = parseFadeOut(jData);
            }
            else
            {
                assert(!"Unknown effect type");
            }

            return effect;
        }

        CEffect_FadeIn* parseFadeIn(const json& jData)
        {
            CEffect_FadeIn* effect = new CEffect_FadeIn();
            float time = jData.value("time", 1.f);
            effect->setFadeTime(time);
            return effect;
        }

        CEffect_FadeOut* parseFadeOut(const json& jData)
        {
            CEffect_FadeOut* effect = new CEffect_FadeOut();
            float time = jData.value("time", 1.f);
            effect->setFadeTime(time);
            return effect;
        }

        CEffect_AnimateUV* parseAnimateUVs(const json& jData)
        {
            CEffect_AnimateUV* effect = new CEffect_AnimateUV();
            const VEC2 speed = loadVEC2(jData, "speed", VEC2::Zero);
            effect->setSpeed(speed);
            return effect;
        }

        void parseBaseParams(CWidget& params, const json& jData)
        {
            params.setVisible(jData.value("visible", true));
            params.setName(jData.value("name", ""));
            params.setAlias(jData.value("alias", ""));
            params.setPivot(loadVEC2(jData, "pivot", VEC2::Zero));
            params.setPosition(loadVEC2(jData, "position", VEC2::Zero));
            params.setAngle(deg2rad(jData.value("angle", 0.f)));
            params.setScale(loadVEC2(jData, "scale", VEC2::One));

            if (jData.count("size") > 0)
            {
                params.setSize(ESizeType::Fixed, loadVEC2(jData, "size", VEC2::Zero));
            }
            else
            {
                params.setSize(ESizeType::Percent, loadVEC2(jData, "size_percent", VEC2::One * 100.f) / 100.f);
            }
        }

        void parseVideoParams(TVideoParams& params, const json& jData)
        {
            assert(jData.count("src") > 0);
            if (jData.count("src") > 0)
            {
                std::string src_name = jData["src"];

                params.video = new VideoTexture();
                bool is_ok = params.video->create(src_name.c_str());
                params.video->setAutoLoop(jData.value("auto_loop", params.auto_loop));
                assert(is_ok);
            }
            
            params.additive = jData.value("additive", params.additive);
            std::string fx = jData.value("fx", std::string());
            if (fx.length() > 0)
            {
                params.fx_pipeline = Resources.get(fx)->as<CPipelineState>();
            }
        }

        void parseImageParams(TImageParams& params, const json& jData)
        {
            params.texture = jData.count("texture") > 0 ? Resources.get(jData["texture"])->as<CTexture>() : params.texture;
            params.cut_texture = Resources.get(jData.count("cut_texture") > 0 ? jData["cut_texture"] : "data/textures/black.dds")->as<CTexture>();
            params.color = loadColor(jData, "color", params.color * 255.f) / 255.f;
            params.minUV = loadVEC2(jData, "min_uv", params.minUV);
            params.maxUV = loadVEC2(jData, "max_uv", params.maxUV);
            params.additive = jData.value("additive", params.additive);
            params.alpha_cut = jData.value("alpha_cut", params.alpha_cut);
            params.hud = jData.value("hud", params.hud);
        }

        void parseFontParams(TFontParams& params, const json& jData)
        {
            params.name = jData.value("name", params.name);
            params.texture = jData.count("texture") > 0 ? Resources.get(jData["texture"])->as<CTexture>() : params.texture;
            params.numRows = jData.value("num_rows", params.numRows);
            params.numCols = jData.value("num_cols", params.numCols);
        }

        void parseTextParams(TTextParams& params, const json& jData)
        {
            const std::string fontName = jData.value("font", params.font ? params.font->name : std::string());

            params.text = jData.value("text", params.text);
            params.color = loadColor(jData, "text_color", params.color * 255.f) / 255.f;
            params.font = CEngine::get().getUI().getFont(fontName);
            params.size = jData.value("font_size", params.size);

            const std::string halignStr = jData.value("text_halign", "");
            if (halignStr == "center")      params.halign = EHAlign::Center;
            else if (halignStr == "right")  params.halign = EHAlign::Right;
            else if (halignStr == "left")   params.halign = EHAlign::Left;

            const std::string valignStr = jData.value("text_valign", "");
            if (valignStr == "center")      params.valign = EVAlign::Center;
            else if (valignStr == "bottom") params.valign = EVAlign::Bottom;
            else if (valignStr == "top")    params.valign = EVAlign::Top;
        }
    }
}