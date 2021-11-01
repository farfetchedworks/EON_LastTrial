#pragma once

#include "mcv_platform.h"

namespace fsm
{
    class CContext;
    class IState;
    class ITransition;

    using VStates = std::vector<const IState*>;
    using VTransitions = std::vector<const ITransition*>;

    using TVariableValue = std::variant<bool, int, float, std::string, VEC3, VEC2>;
}
