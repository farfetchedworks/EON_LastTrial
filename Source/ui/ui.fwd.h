#pragma once

namespace ui
{
    class CWidget;
    class CImage;
    class CVideo;
    class CText;
    class CProgressBar;
    class CButton;

    struct TImageParams;
    struct TVideoParams;
    struct TFontParams;
    struct TTextParams;

    class CEffect;
    class CEffect_AnimateUV;
    class CEffect_FadeIn;
    class CEffect_FadeOut;

    constexpr float kDebugMenuSensibility = 0.05f;

    using Callback = std::function<void()>;

    enum class ESizeType
    {
        Fixed = 0,
        Percent
    };

    enum class EHAlign
    {
        Left = 0,
        Center,
        Right
    };

    enum class EVAlign
    {
        Top = 0,
        Center,
        Bottom
    };
}
