#pragma once

#include "mcv_platform.h"

namespace ui
{
    class IController
    {
      public:
        virtual void update(float elapsed) {}
    };
}
