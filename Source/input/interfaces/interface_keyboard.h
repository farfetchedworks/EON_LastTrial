#pragma once

#include "input/input_button.h"

namespace input
{
    constexpr int NUM_KEYBOARD_KEYS = 256;
    using EKeyboardKey = int;

    struct TKeyboardData
    {
        std::array<TButton, NUM_KEYBOARD_KEYS> keys = {};

        void update(float dt)
        {
          PROFILE_FUNCTION("Keyboard.update");
            for (auto& key : keys)
            {
                key.update(dt);
            }
        }
    };
}
