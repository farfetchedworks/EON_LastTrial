#pragma once

#include "input/interfaces/interface_keyboard.h"
#include "input/interfaces/interface_mouse.h"
#include "input/interfaces/interface_pad.h"
#include "input/interfaces/interface_vibration.h"

namespace input
{
    class IDevice;
    using VDevices = std::vector<IDevice*>;

    enum Players
    {
        PLAYER_1 = 0,
        MENU,
        MAX_PLAYERS
    };

    enum class EInterface
    {
        KEYBOARD = 0,
        MOUSE,
        PAD
    };

    struct TButtonDef
    {
        EInterface type = EInterface::KEYBOARD;
        int id = 0;

        bool operator==(const TButtonDef& other) const
        {
            return type == other.type && id == other.id;
        }
    };
}
