#pragma once
#include "render/textures/video/video_texture.h"
#include "ui/ui.fwd.h"

namespace ui
{
    struct TVideoParams
    {
        VideoTexture* video = nullptr;
        bool auto_loop = true;
        bool additive = false;
        float time_normalized = 1.f;

        void renderInMenu();
    };

    struct TImageParams
    {
        Color color = Color::One;
        const CTexture* texture = nullptr;
        const CTexture* cut_texture = nullptr;
        VEC2 minUV = VEC2::Zero;
        VEC2 maxUV = VEC2::One;
        bool additive = false;
        bool hud = false;
        float alpha_cut = 1.f;
        float time_normalized = 1.f;

        void renderInMenu();
    };

    struct TFontParams
    {
        std::string name;
        const CTexture* texture = nullptr;
        int numRows = 1;
        int numCols = 1;

        void renderInMenu();
    };

    struct TTextParams
    {
        std::string text;
        Color color = Color::One;
        const TFontParams* font = nullptr;
        int size = 16;
        EHAlign halign = EHAlign::Left;
        EVAlign valign = EVAlign::Top;

        void renderInMenu();
    };
}
