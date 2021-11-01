#pragma once

#include "input/input_button.h"

namespace input
{
    enum class EPadButton
    {
        A = 0,
        B,
        X,
        Y,
        L1,
        L2,
        LTHUMB,
        R1,
        R2,
        RTHUMB,
        OPTIONS,
        START,
        LANALOG_X,
        LANALOG_Y,
        RANALOG_X,
        RANALOG_Y,
        DPAD_LEFT,
        DPAD_RIGHT,
        DPAD_UP,
        DPAD_DOWN,
        NUM_BUTTONS
    };
    constexpr int NUM_PAD_BUTTONS = static_cast<int>(EPadButton::NUM_BUTTONS);

    struct TPadData
    {
        bool connected = false;
        TButton buttons[NUM_PAD_BUTTONS];
        
        void update(float dt)
        {
          PROFILE_FUNCTION("Pad.update");
            for (auto& bt : buttons)
            {
                bt.update(dt);
            }
        }
    };

    inline bool isAnalog(EPadButton bt)
    {
        return bt == EPadButton::LANALOG_X || bt == EPadButton::LANALOG_Y || bt == EPadButton::RANALOG_X || bt == EPadButton::RANALOG_Y;
    }
}
