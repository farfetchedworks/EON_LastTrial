#include "mcv_platform.h"
#include "input/input_mapping.h"
#include "input/input_module.h"

namespace input
{
    CMapping::CMapping(CModule& module)
    : _module(module)
    {

    }

    void CMapping::load(const std::string& filename)
    {
        auto jData = loadJson(filename);
        for (const auto& jEntry : jData)
        {
            const std::string& name = jEntry["name"];
            const json& jButtons = jEntry["buttons"];
            const std::string& op = jEntry["operation"];

            TMappedButton mbt;

            for (const auto& jButton : jButtons)
            {
                TMappedButton::TSourceButton srcButton;

                std::string buttonName = jButton;
                if (buttonName[0] == '-')
                {
                    srcButton.negated = true;
                    buttonName = buttonName.substr(1);
                }

                srcButton.button = &_module.getButton(buttonName);

                mbt.sourceButtons.push_back(srcButton);
            }

            if(op == "any")                 mbt.operation = TMappedButton::EOperation::ANY;
            else if(op == "all")            mbt.operation = TMappedButton::EOperation::ALL;
            else if(op == "vector")         mbt.operation = TMappedButton::EOperation::VECTORIZE;
            else if (op == "vector_pad")    mbt.operation = TMappedButton::EOperation::VECTORIZE_PAD;
            
            _mappedButtons[name] = mbt;
        }
    }

    void CMapping::update(float dt)
    {
        PROFILE_FUNCTION("Mapping");
        for (auto& entry : _mappedButtons)
        {
            TMappedButton& mbt = entry.second;

            float value = mbt.operation == TMappedButton::EOperation::ALL ? 1.f : -1.f;
            VEC2 vector = VEC2(0.f, 0.f);

            int idx = 0;
            for (const TMappedButton::TSourceButton& bt : mbt.sourceButtons)
            {
                float btValue = bt.button->value;
                if(bt.negated)
                {
                    btValue *= -1.f;
                }

                switch (mbt.operation)
                {
                    case TMappedButton::EOperation::ANY:
                    {
                        value = std::max(value, btValue);
                        break;
                    }
                    case TMappedButton::EOperation::ALL:
                    {
                        value = std::min(value, btValue);
                        break;
                    }
                    case TMappedButton::EOperation::VECTORIZE:
                    {
                        float& axis = (idx % 2) == 0 ? vector.x : vector.y;
                        axis = std::max(value, btValue);
                        value = vector.LengthSquared();
                        break;
                    }
                    case TMappedButton::EOperation::VECTORIZE_PAD:
                    {
                        float& axis = (idx % 2) == 0 ? vector.x : vector.y;
                        axis = btValue;
                        value = vector.LengthSquared();
                        break;
                    }
                }

                ++idx;
            }

            mbt.button.setValue(value);
            mbt.button.update(dt);
        }
    }

    void CMapping::clear()
    {
        for (auto& entry : _mappedButtons)
        {
            TMappedButton& mbt = entry.second;
            mbt.button.setValue(0.f);
        }
    }

    const TButton& CMapping::getButton(const std::string& name) const
    {
        if (_module.isBlocked())
            return TButton::dummy;

        auto it = _mappedButtons.find(name);
        return it != _mappedButtons.cend() ? it->second.button : TButton::dummy;
    }

    VEC2 CMapping::getDirection(const std::string& name) const
    {
        auto it = _mappedButtons.find(name);
        return it != _mappedButtons.cend() ? it->second.vector : VEC2::Zero;
    }
}
