#pragma once

#include "mcv_platform.h"

namespace input
{
    struct TKeyboardData;
    struct TMouseData;
    struct TPadData;
    struct TVibrationData;

    class IDevice
    {
      public:
        virtual void fetchData(TKeyboardData& data){}
        virtual void fetchData(TMouseData& data) {};
        virtual void fetchData(TPadData& data) {};
        virtual void feedback(const TVibrationData& data) {};
        virtual void clearData() {};
    };
}
