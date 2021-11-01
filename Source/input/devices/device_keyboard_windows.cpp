#include "mcv_platform.h"
#include "engine.h"
#include "input/devices/device_keyboard_windows.h"

namespace input
{
    void CDeviceKeyboardWindows::fetchData(TKeyboardData& data)
    {
        for (int i = 0; i < NUM_KEYBOARD_KEYS; i++)
        {
            //const bool pressed = isPressed(i); // polling
            const bool pressed = _keys[i];
            data.keys[i].setValue(pressed ? 1.f : 0.f);
        }
    }

    void CDeviceKeyboardWindows::clearData()
    {
        _keys.reset();
    }

    void CDeviceKeyboardWindows::processMsg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureKeyboard) return;

        switch (message)
        {
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
            {
                _keys[(int)wParam] = true;
                break;
            }
        case WM_SYSKEYUP:
        case WM_KEYUP:
            {
                _keys[(int)wParam] = false;
                break;
            }
        }
    }
}
