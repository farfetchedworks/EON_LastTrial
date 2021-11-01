#pragma once

#include "fsm/fsm.fwd.h"

namespace fsm
{
    class CVariable
    {
    public:
        void load(const json& jData);

        void setValue(const TVariableValue& value){ _value = value; }

        const std::string& getName() const { return _name; }
        const TVariableValue& getValue() const { return _value; }

        std::string toString() const;
        void renderInMenu() const;
        void renderInMenu();

        static TVariableValue parseValue(const std::string& str);
        
    private:
        std::string _name;
        TVariableValue _value;
    };

    using MVariables = std::map<std::string, CVariable>;
}
