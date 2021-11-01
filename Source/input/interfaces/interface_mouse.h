#pragma once

#include "input/input_button.h"

namespace input
{
    enum class EMouseButton
    {
        LEFT = 0,
        MIDDLE,
        RIGHT,
        NUM_BUTTONS
    };
    constexpr int NUM_MOUSE_BUTTONS = static_cast<int>(EMouseButton::NUM_BUTTONS);

    struct TMouseData
    {
        TButton buttons[3];
        VEC2 position = VEC2::Zero;
        VEC2 delta = VEC2::Zero;
        int wheelSteps = 0;

        void update(float dt)
        {
          PROFILE_FUNCTION("Mouse.update");
            for (auto& bt : buttons)
            {
                bt.update(dt);
            }
        }
    };
}
