#pragma once

#include "mcv_platform.h"

namespace input
{
    struct TButton
    {
        static constexpr float kUnpressed = 0.f;
        static const TButton dummy;

        float value = kUnpressed;
        float prevValue = kUnpressed;
        float time = 0.f;
        float timeReleased = 0.f;
        float prevTime = 0.f;

        void update(float dt)
        {
            prevTime = time;
            if (isPressed())
            {
                time += dt;
                timeReleased = 0.f;
            }
            else
            {
                time = 0.f;
                timeReleased += dt;
            }
        }

        void setValue(float newValue)
        {
            prevValue = value;
            value = newValue;
        }

        bool isPressed() const
        {
            return value != 0.f;
        }

        bool wasPressed() const
        {
            return prevValue != 0.f;
        }

        bool getsPressed() const
        {
            return !wasPressed() && isPressed();
        }

        bool getsReleased() const
        {
            return wasPressed() && !isPressed();
        }

        float timeSincePressed() const
        {
            return time;
        }

        float prevTimeSincePressed() const
        {
            return prevTime;
        }

        float timeSinceReleased() const
        {
            return timeReleased;
        }
    };
}
