#pragma once

#include "modules/module.h"
#include "input/input.fwd.h"

namespace input
{
    class CModule;

    struct TMappedButton
    {
        enum class EOperation{ ANY, ALL, VECTORIZE, VECTORIZE_PAD };
        struct TSourceButton
        {
            const TButton* button = nullptr;
            bool negated = false;
        };
        
        EOperation operation = EOperation::ANY;
        std::vector<TSourceButton> sourceButtons;
        TButton button;
        VEC2 vector;
    };

    class CMapping
    {
      public:
        CMapping(CModule& module);
        void load(const std::string& filename);
        void update(float dt);
        
        const TButton& getButton(const std::string& name) const;
        VEC2 getDirection(const std::string& name) const;
        const std::map<std::string, TMappedButton>& getMappedButtons() const { return _mappedButtons; }

      private:
        CModule& _module;
        std::map<std::string, TMappedButton> _mappedButtons;
    };
}
