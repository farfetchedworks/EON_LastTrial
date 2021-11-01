#include "mcv_platform.h"
#include "engine.h"
#include "input/devices/device_pad_xbox_windows.h"
#include "input/interfaces/interface_vibration.h"
#include <Xinput.h>

namespace input
{
    float readButton(XINPUT_STATE& state, WORD buttonMask)
    {
        return (state.Gamepad.wButtons & buttonMask) != 0 ? 1.f : 0.f;
    }

    float readTrigger(XINPUT_STATE& state, bool right)
    {
        BYTE value = right ? state.Gamepad.bRightTrigger : state.Gamepad.bLeftTrigger;
        BYTE deadZone = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
        if (value <= deadZone)
        {
            return 0.f;
        }

        return static_cast<float>(value - deadZone) / static_cast<float>(std::numeric_limits<BYTE>::max() - deadZone);
    }

    void readAnalog(SHORT xAxis, SHORT yAxis, float& x, float& y, bool right)
    {
        const SHORT deadzone = right ? XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE : XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;

        float LX = xAxis;
        float LY = yAxis;

        if (xAxis == 0 && yAxis == 0) {
            x = 0;
            y = 0;
            return;
        }

        //determine how far the controller is pushed
        float magnitude = sqrt(LX * LX + LY * LY);
        
        //determine the direction the controller is pushed
        float normalizedLX = LX / magnitude;
        float normalizedLY = LY / magnitude;

        float normalizedMagnitude = 0;

        //check if the controller is outside a circular dead zone
        if (magnitude > deadzone)
        {
            //clip the magnitude at its expected maximum value
            if (magnitude > 32767) magnitude = 32767;

            //adjust magnitude relative to the end of the dead zone
            magnitude -= deadzone;

            //optionally normalize the magnitude with respect to its expected range
            //giving a magnitude value of 0.0 to 1.0
            normalizedMagnitude = magnitude / (32767 - deadzone);
        }
        else //if the controller is in the deadzone zero out the magnitude
        {
            magnitude = 0.0;
            normalizedMagnitude = 0.0;
        }


        x = normalizedLX * normalizedMagnitude;
        y = normalizedLY * normalizedMagnitude;
    }

    CDevicePadXboxWindows::CDevicePadXboxWindows(int id)
        : _id(id)
    {}

    void CDevicePadXboxWindows::fetchData(TPadData& data)
    {
        updateController();

        data.connected = _connected;
        for (int i = 0; i < NUM_PAD_BUTTONS; i++)
        {
            data.buttons[i].setValue(_buttons[i]);
        }
    }


    void CDevicePadXboxWindows::updateController()
    {
        XINPUT_STATE state;
        ZeroMemory(&state, sizeof(XINPUT_STATE));

        // Simply get the state of the controller from XInput.
        const DWORD dwResult = XInputGetState(_id, &state);
        _connected = dwResult == ERROR_SUCCESS;
        
        if (!_connected)
        {
            return;
        }

        _buttons[static_cast<int>(EPadButton::A)] = readButton(state, XINPUT_GAMEPAD_A);
        _buttons[static_cast<int>(EPadButton::B)] = readButton(state, XINPUT_GAMEPAD_B);
        _buttons[static_cast<int>(EPadButton::X)] = readButton(state, XINPUT_GAMEPAD_X);
        _buttons[static_cast<int>(EPadButton::Y)] = readButton(state, XINPUT_GAMEPAD_Y);
        _buttons[static_cast<int>(EPadButton::OPTIONS)] = readButton(state, XINPUT_GAMEPAD_BACK);
        _buttons[static_cast<int>(EPadButton::START)] = readButton(state, XINPUT_GAMEPAD_START);
        _buttons[static_cast<int>(EPadButton::DPAD_DOWN)] = readButton(state, XINPUT_GAMEPAD_DPAD_DOWN);
        _buttons[static_cast<int>(EPadButton::DPAD_UP)] = readButton(state, XINPUT_GAMEPAD_DPAD_UP);
        _buttons[static_cast<int>(EPadButton::DPAD_LEFT)] = readButton(state, XINPUT_GAMEPAD_DPAD_LEFT);
        _buttons[static_cast<int>(EPadButton::DPAD_RIGHT)] = readButton(state, XINPUT_GAMEPAD_DPAD_RIGHT);
        _buttons[static_cast<int>(EPadButton::L1)] = readButton(state, XINPUT_GAMEPAD_LEFT_SHOULDER);
        _buttons[static_cast<int>(EPadButton::R1)] = readButton(state, XINPUT_GAMEPAD_RIGHT_SHOULDER);
        _buttons[static_cast<int>(EPadButton::LTHUMB)] = readButton(state, XINPUT_GAMEPAD_LEFT_THUMB);
        _buttons[static_cast<int>(EPadButton::RTHUMB)] = readButton(state, XINPUT_GAMEPAD_RIGHT_THUMB);

        _buttons[static_cast<int>(EPadButton::L2)] = readTrigger(state, false);
        _buttons[static_cast<int>(EPadButton::R2)] = readTrigger(state, true);

        float x, y;
        readAnalog(state.Gamepad.sThumbLX, state.Gamepad.sThumbLY, x, y, false);
        _buttons[static_cast<int>(EPadButton::LANALOG_X)] = x;
        _buttons[static_cast<int>(EPadButton::LANALOG_Y)] = y;
        
        readAnalog(state.Gamepad.sThumbRX, state.Gamepad.sThumbRY, x, y, true);
        _buttons[static_cast<int>(EPadButton::RANALOG_X)] = x;
        _buttons[static_cast<int>(EPadButton::RANALOG_Y)] = y;
    }

    void CDevicePadXboxWindows::feedback(const TVibrationData& data)
    {
        XINPUT_VIBRATION vibration;
        ZeroMemory(&vibration, sizeof(XINPUT_VIBRATION));

        vibration.wLeftMotorSpeed = static_cast<WORD>(data.leftRatio * std::numeric_limits<WORD>::max());
        vibration.wRightMotorSpeed = static_cast<WORD>(data.rightRatio * std::numeric_limits<WORD>::max());

        XInputSetState(_id, &vibration);
    }
    void CDevicePadXboxWindows::clearData()
    {
        _buttons.fill(0.f);
    }
}
