#include "mcv_platform.h"
#include "fsm/fsm_variable.h"

namespace fsm
{
    void CVariable::load(const json& jData)
    {
        _name = jData["name"];

        const std::string type = jData["type"];
        if(type == "bool")
        {
            _value = jData.value("default_value", false);
        }
        else if (type == "int")
        {
            _value = jData.value("default_value", 0);
        }
        else if (type == "float")
        {
            _value = jData.value("default_value", 0.f);
        }
        else if (type == "string")
        {
            _value = jData.value("default_value", std::string());
        }
        else if (type == "vec2")
        {
            _value = loadVEC2(jData, "default_value", VEC2::Zero);
        }
        else if (type == "vec3")
        {
            _value = loadVEC3(jData, "default_value");
        }
    }

    std::string CVariable::toString() const
    {
        if (std::holds_alternative<bool>(_value))
        {
            return std::get<bool>(_value) ? "True" : "False";
        }
        else if (std::holds_alternative<int>(_value))
        {
            return std::to_string(std::get<int>(_value));
        }
        else if (std::holds_alternative<float>(_value))
        {
            return std::to_string(std::get<float>(_value));
        }
        else if (std::holds_alternative<std::string>(_value))
        {
            return std::get<std::string>(_value);
        }
        else
        {
            return std::string();
        }
    }

    TVariableValue CVariable::parseValue(const std::string& str)
    {
        // consider forcing lowcase!!
        if (str == "false" || str == "False" || str == "FALSE")
        {
            return false;
        }
        else if (str == "true" || str == "True" || str == "TRUE")
        {
            return true;
        }
        else if (isNumber(str))
        {
            if (str.find('.') != std::string::npos)
            {
                return std::stof(str);
            }
            else
            {
                return std::stoi(str);
            }
        }
        else
        {
            return str;
        }
    }

    void CVariable::renderInMenu() const
    {
        ImGui::Text("%s: %s", _name.c_str(), toString().c_str());
    }

    void CVariable::renderInMenu()
    {
        if (std::holds_alternative<bool>(_value))
        {
            bool& value = std::get<bool>(_value);
            ImGui::Checkbox(_name.c_str(), &value);
        }
        else if (std::holds_alternative<int>(_value))
        {
            int& value = std::get<int>(_value);
            ImGui::DragInt(_name.c_str(), &value);
        }
        else if (std::holds_alternative<float>(_value))
        {
            float& value = std::get<float>(_value);
            ImGui::DragFloat(_name.c_str(), &value);
        }
        else if (std::holds_alternative<std::string>(_value))
        {
            std::string& value = std::get<std::string>(_value);
            static char buffer[64];
            snprintf(buffer, 63, value.c_str());
            if (ImGui::InputText(_name.c_str(), buffer, 63))
            {
                value = buffer;
            }
        }
    }
}