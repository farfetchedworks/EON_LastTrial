#pragma once

#include "input/devices/device_windows.h"
#include "input/input_button.h"
#include "input/interfaces/interface_keyboard.h"

namespace input
{
    class CDeviceKeyboardWindows : public IDeviceWindows
    {
    public:
        void fetchData(TKeyboardData& data) override;
        void processMsg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) override;
        void clearData() override;

    private:
        std::bitset<NUM_KEYBOARD_KEYS> _keys;
    };
}
