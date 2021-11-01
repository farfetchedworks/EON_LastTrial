#include "mcv_platform.h"
#include "fsm/transitions/transition_check_variable.h"
#include "fsm/fsm_context.h"

namespace fsm
{
    bool CTransitionCheckVariable::onCheck(CContext& ctx) const
    {
        const CVariable* lVar = ctx.getVariable(_lName);
        const CVariable* rVar = ctx.getVariable(_rName);

        const TVariableValue& lValue = lVar ? lVar->getValue() : _lValue;
        const TVariableValue& rValue = rVar ? rVar->getValue() : _rValue;

        // different types? (possibly because some var is not defined)        
        if(lValue.index() != rValue.index())
            return false;

        switch (_operation)
        {
            case EOperation::LESS:          return lValue < rValue;
            case EOperation::LESS_EQUAL:    return lValue <= rValue;
            case EOperation::EQUAL:         return lValue == rValue;
            case EOperation::NOT_EQUAL:     return lValue != rValue;
            case EOperation::GREATER_EQUAL: return lValue >= rValue;
            case EOperation::GREATER:       return lValue > rValue;
            default:;
        }
        
        return false;
    }

    void CTransitionCheckVariable::onLoad(const json& params)
    {
        const std::string& condition = params["condition"];
        const std::vector<std::string> tokens = split(condition, ' ');
        assert(tokens.size() == 3);

        _lName = tokens.at(0);
        _rName = tokens.at(2);

        _lValue = CVariable::parseValue(_lName);
        _rValue = CVariable::parseValue(_rName);

        const std::string& op = tokens.at(1);
        if(op == "<")           _operation = EOperation::LESS;
        else if(op == "<=")     _operation = EOperation::LESS_EQUAL;
        else if(op == "==")     _operation = EOperation::EQUAL;
        else if(op == "!=")     _operation = EOperation::NOT_EQUAL;
        else if(op == ">=")     _operation = EOperation::GREATER_EQUAL;
        else if(op == ">")      _operation = EOperation::GREATER;

        blend_time = params.value("blend_time", blend_time);
    }

    void CTransitionCheckVariable::renderInMenu() const
    {
        const char* opStr = "";
        switch (_operation)
        {
        case EOperation::LESS:          { opStr = "<";  break; }
        case EOperation::LESS_EQUAL:    { opStr = "<=";  break; }
        case EOperation::EQUAL:         { opStr = "==";  break; }
        case EOperation::NOT_EQUAL:     { opStr = "!=";  break; }
        case EOperation::GREATER_EQUAL: { opStr = ">=";  break; }
        case EOperation::GREATER:       { opStr = ">";  break; }
        default:;
        }

        if (ImGui::TreeNode(this, "[%s] %s -> %s", type.data(), source ? source->name.c_str() : "...", target ? target->name.c_str() : "..."))
        {
            ImGui::Text("Condition: %s %s %s", _lName.c_str(), opStr, _rName.c_str());
            ImGui::TreePop();
        }
    }
}
