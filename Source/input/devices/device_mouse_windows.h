#pragma once

#include "input/devices/device_windows.h"
#include "input/input_button.h"
#include "input/interfaces/interface_mouse.h"

namespace input
{
    class CDeviceMouseWindows : public IDeviceWindows
    {
    public:
        CDeviceMouseWindows(HWND hWnd);
        void fetchData(TMouseData& data) override;
        void processMsg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) override;
        void clearData() override;

    private:
        std::bitset<NUM_MOUSE_BUTTONS> _buttons;

        VEC2 _currPosition = VEC2::Zero;
        VEC2 _delta = VEC2::Zero;
        int _wheelSteps = 0;
    };
}
