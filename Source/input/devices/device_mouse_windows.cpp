#include "mcv_platform.h"
#include "engine.h"
#include "input/devices/device_mouse_windows.h"
#include <windowsx.h>
#include "windows/application.h"

#ifndef HID_USAGE_PAGE_GENERIC
#define HID_USAGE_PAGE_GENERIC         ((USHORT) 0x01)
#endif
#ifndef HID_USAGE_GENERIC_MOUSE
#define HID_USAGE_GENERIC_MOUSE        ((USHORT) 0x02)
#endif

namespace input
{
    CDeviceMouseWindows::CDeviceMouseWindows(HWND hWnd)
    {
        RAWINPUTDEVICE Rid[1];
        Rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
        Rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;
        Rid[0].dwFlags = RIDEV_INPUTSINK;
        Rid[0].hwndTarget = hWnd;
        RegisterRawInputDevices(Rid, 1, sizeof(Rid[0]));
    }

    void CDeviceMouseWindows::fetchData(TMouseData& data)
    {
        for (int i = 0; i < NUM_MOUSE_BUTTONS; i++)
        {
            data.buttons[i].setValue(_buttons[i] ? 1.f : 0.f);
        }

        data.position = _currPosition;
        data.wheelSteps = _wheelSteps;

        if (CApplication::get().getMouseCentered()) {

            data.delta = _currPosition - VEC2(0.5);

            if (data.delta.x != 0 || data.delta.y != 0) {
                centerMousePos();
            }
        }
        else {
            data.delta = _delta;
        }

        _delta = VEC2::Zero;
        _wheelSteps = 0;
    }

    void CDeviceMouseWindows::processMsg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse) return;

        switch (message)
        {
            case WM_LBUTTONDOWN:
            {
                _buttons[0] = true;
                SetCapture(hWnd);
                break;
            }
            case WM_LBUTTONUP:
            {
                _buttons[0] = false;
                ReleaseCapture();
                break;
            }
            case WM_MBUTTONDOWN:
            {
                _buttons[1] = true;
                SetCapture(hWnd);
                break;
            }
            case WM_MBUTTONUP:
            {
                _buttons[1] = false;
                ReleaseCapture();
                break;
            }
            case WM_RBUTTONDOWN:
            {
                _buttons[2] = true;
                SetCapture(hWnd);
                //ShowCursor(false);
                break;
            }
            case WM_RBUTTONUP:
            {
                _buttons[2] = false;
                ReleaseCapture();
                //ShowCursor(true);
                break;
            }
            case WM_MOUSEMOVE:
            {
                VEC2 dt = _delta;
                if (CApplication::get().getMouseCentered() && dt.x == 0 && dt.y == 0) {
                    _currPosition = VEC2(0.5);
                }
                else {
                    int width, height;
                    CApplication::get().getDimensions(width, height);
                    _currPosition.x = static_cast<float>(GET_X_LPARAM(lParam)) / static_cast<float>(width);
                    _currPosition.y = static_cast<float>(GET_Y_LPARAM(lParam)) / static_cast<float>(height);
                }
                break;
            }
            case WM_MOUSEWHEEL:
            {
                _wheelSteps = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
                break;
            }
            case WM_INPUT: {
                UINT dwSize = 64;
                static BYTE lpb[64];

                int rc = ::GetRawInputData((HRAWINPUT)lParam, RID_INPUT,
                    lpb, &dwSize, sizeof(RAWINPUTHEADER));
                if (rc == -1)
                    break;

                RAWINPUT* raw = (RAWINPUT*)lpb;
                if (raw->header.dwType == RIM_TYPEMOUSE)
                {
                    int width, height;
                    CApplication::get().getDimensions(width, height);

                    _delta.x = static_cast<float>(raw->data.mouse.lLastX) / static_cast<float>(width);
                    _delta.y = static_cast<float>(raw->data.mouse.lLastY) / static_cast<float>(height);
                }
                break;
            }
            default:;
        }
    }   
    void CDeviceMouseWindows::clearData()
    {
        _delta = VEC2::Zero;
        _currPosition = VEC2(0.5);
        _wheelSteps = 0;
        _buttons.reset();
    }
}
