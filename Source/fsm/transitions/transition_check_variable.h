#pragma once

#include "fsm/fsm_transition.h"
#include "fsm/fsm_variable.h"

namespace fsm
{
    class CTransitionCheckVariable : public ITransition
    {
    public:
        void onLoad(const json& params) override;
        bool onCheck(CContext& ctx) const override;
        void renderInMenu() const override;

    private:
        enum class EOperation
        {
            LESS, LESS_EQUAL, EQUAL, NOT_EQUAL, GREATER_EQUAL, GREATER 
        };

        EOperation _operation = EOperation::EQUAL;
        std::string _lName;
        std::string _rName;
        TVariableValue _lValue;
        TVariableValue _rValue;
    };
}