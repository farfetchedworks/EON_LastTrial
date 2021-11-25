#pragma once

#include "ui.fwd.h"

namespace ui
{
    namespace parser
    {
        void parseWidgetsFileList(const std::string& filename);
        void parseWidgetsFile(const std::string& filename);
        void parseFonts(const std::string& filename);

        CWidget* parseWidget(const json& jData);
        CWidget* parseWidget(const json& jData);
        CWidget* parseImage(const json& jData);
        CWidget* parseVideo(const json& jData);
        CWidget* parseText(const json& jData);
        CWidget* parseProgressBar(const json& jData);
        CWidget* parseButton(const json& jData);
        CWidget* parseCheckbox(const json& jData);

        CEffect* parseEffect(const json& jData);
        CEffect_AnimateUV* parseAnimateUVs(const json& jData);
        CEffect_FadeIn* parseFadeIn(const json& jData);
        CEffect_FadeOut* parseFadeOut(const json& jData);

        void parseBaseParams(CWidget& params, const json& jData);
        void parseImageParams(TImageParams& params, const json& jData);
        void parseVideoParams(TVideoParams& params, const json& jData);
        void parseFontParams(TFontParams& params, const json& jData);
        void parseTextParams(TTextParams& params, const json& jData);
    }
}
