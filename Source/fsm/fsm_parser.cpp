#include "mcv_platform.h"
#include "fsm/fsm_parser.h"
#include "fsm/fsm.h"
#include "fsm/states/state_shake.h"
#include "fsm/states/state_move.h"
#include "fsm/states/anim/state_animation.h"
#include "fsm/states/anim/state_blend_animation.h"
#include "fsm/transitions/transition_wait_time.h"
#include "fsm/transitions/transition_wait_state_finished.h"
#include "fsm/transitions/transition_check_variable.h"

namespace fsm
{
    std::map<std::string, CParser::IStateFactory*> state_registry = {};
    std::map<std::string_view, CParser::IStateFactory*> CParser::_stateTypes;
    std::map<std::string_view, CParser::ITransitionFactory*> CParser::_transitionTypes;

    void CParser::registerTypes()
    {
        _stateTypes[""] = new CStateFactory<CStateDummy>();
        _stateTypes["any"] = new CStateFactory<CStateDummy>();
        _stateTypes["shake"] = new CStateFactory<CStateShake>();
        _stateTypes["move"] = new CStateFactory<CStateMove>();

        _stateTypes["animation"] = new CStateFactory<CStateAnimation>();
        _stateTypes["blend_animation"] = new CStateFactory<CStateBlendAnimation>();

        for (auto& state : state_registry) {
            const std::string& name = state.first;
            _stateTypes[name] = state.second;
        }

        /*
            TRANSITIONS
        */

        _transitionTypes["wait_time"] = new CTransitionFactory<CTransitionWaitTime>();
        _transitionTypes["wait_state_finished"] = new CTransitionFactory<CTransitionWaitStateFinished>();
        _transitionTypes["check_variable"] = new CTransitionFactory<CTransitionCheckVariable>();
    }

    IState* CParser::createState(const std::string& type)
    {
        auto it = _stateTypes.find(type);
        if (it == _stateTypes.cend())
        {
            fatal("Undefined FSM state type %s", type.c_str());
            return nullptr;
        }

        IState* st = it->second->create();
        st->type = it->first;
        return st;
    }

    ITransition* CParser::createTransition(const std::string& type)
    {
        auto it = _transitionTypes.find(type);
        if (it == _transitionTypes.cend())
        {
            fatal("Undefined FSM transition type %s", type.c_str());
            return nullptr;
        }

        ITransition* tr = it->second->create();
        tr->type = it->first;
        return tr;
    }

    bool CParser::parse(CFSM* fsm, const json& jFile)
    {
        // states
        const json& jStates = jFile["states"];
        for (const auto& jState : jStates)
        {
            bool isNewState = false;
            const std::string& name = jState.value("name", std::string());

            IState* newState = const_cast<IState*>(fsm->getState(name));

            if (newState == nullptr) {
                isNewState = true;
                const std::string type = jState.value("type", std::string());
                newState = CParser::createState(type);
            }

            newState->transitions.clear();
            newState->name = name;
            newState->onLoad(jState);

            if (isNewState) {
                fsm->_states.push_back(newState);
            }
        }

        // transitions
        const json& jTransitions = jFile["transitions"];
        for (const auto& jTransition : jTransitions)
        {
            bool isNewTransition = false;
            const std::string& sourceName = jTransition["source"];
            const std::string& targetName = jTransition["target"];

            IState* sourceState = const_cast<IState*>(fsm->getState(sourceName));
            IState* targetState = const_cast<IState*>(fsm->getState(targetName));

            assert(sourceState && targetState);
            if (!sourceState || !targetState) continue;

            const std::string type          = jTransition.value("type", std::string());
            
            ITransition* newTransition = const_cast<ITransition*>(fsm->getTransition(sourceName, targetName));
            
            if (newTransition == nullptr) {
                isNewTransition = true;
                newTransition = CParser::createTransition(type);
            }

            newTransition->onLoad(jTransition);

            newTransition->source = sourceState;
            newTransition->target = targetState;

            sourceState->transitions.push_back(newTransition);

            if (isNewTransition) {
                fsm->_transitions.push_back(newTransition);
            }
        }

        // variables
        const json& jVariables = jFile["variables"];
        for (const auto& jVariable : jVariables)
        {
            CVariable var;
            var.load(jVariable);
            fsm->_variables[var.getName()] = var;
        }

        // initial state
        fsm->_initialState = fsm->getState(jFile.value("initial_state", std::string()));

        // any state
        fsm->_anyState = fsm->getState("Any");

        return true;
    }

    CStateRegistration::CStateRegistration(std::string module_name, CParser::IStateFactory* creator) {
        state_registry[module_name] = creator;
    }
}