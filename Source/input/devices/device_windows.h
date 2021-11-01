#pragma once

#include "mcv_platform.h"
#include "input/input_device.h"

namespace input
{
    class IDeviceWindows : public IDevice
    {
    public:
        virtual void processMsg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {};
    };
}
