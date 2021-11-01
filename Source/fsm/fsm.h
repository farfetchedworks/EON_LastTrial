#pragma once

#include "resources/resources.h"
#include "fsm.fwd.h"
#include "fsm_variable.h"
#include "fsm/fsm_parser.h"

namespace fsm
{
    class CFSM : public IResource
    {
    public:
        friend class CParser;
        friend class CContext;

        bool renderInMenu() const override;
        bool renderInMenu(CContext& ctx) const;

        const IState* getInitialState() const { return _initialState; }
        const IState* getAnyState() const { return _anyState; }
        const IState* getState(const std::string& name) const;
        const ITransition* getTransition(const std::string& sourceName, const std::string& targetName)  const;
        const MVariables& getVariables() const { return _variables; }
        void onFileChange(const std::string& filename) override;

    private:
        VStates _states;
        VTransitions _transitions;
        const IState* _initialState = nullptr;
        const IState* _anyState = nullptr;
        MVariables _variables;
    };
}