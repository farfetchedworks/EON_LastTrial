#pragma once

#include "input/input_device.h"
#include "input/input_button.h"
#include "input/interfaces/interface_pad.h"

namespace input
{
    class CDevicePadXboxWindows : public IDevice
    {
    public:
        CDevicePadXboxWindows(int id);
        void fetchData(TPadData& data) override;
        void feedback(const TVibrationData& data) override;
        void clearData() override;

    private:
        void updateController();

        int _id = 0;
        bool _connected = false;
        std::array<float, NUM_PAD_BUTTONS> _buttons = {};
    };
}
