#include "mcv_platform.h"
#include "fsm/fsm_context.h"
#include "fsm/states/logic/state_logic.h"
#include "fsm/fsm.h"

namespace fsm
{
    void CContext::setEnabled(bool enabled)
    {
        _enabled = enabled;
    }

    void CContext::update(float dt)
    {
        if (!_enabled || !_active || !_currentState) return;

        _timeInState += dt;
        _currentState->onUpdate(*this, dt);

        // check transitions as many times as need as long as we keep
        // changing the current state
        bool stateChanged = true;
        while (stateChanged)
        {
            stateChanged = checkTransitions();
        }
    }

    bool CContext::checkTransitions()
    {
        if(!_currentState) return false;

        for (const ITransition* transition : _currentState->transitions)
        {
            const bool finished = transition->onCheck(*this);
            if (finished)
            {
                changeState(transition);
                return true;
            }
        }

        const IState* any_state = _fsm->getAnyState();

        if (!any_state) return false;

        // check any_state transitions
        for (const ITransition* transition : any_state->transitions)
        {
            const bool finished = transition->onCheck(*this);
            const std::string& targetName = transition->target->name;
            if (finished && _currentState->name != targetName)
            {
                changeState(transition);
                return true;
            }
        }

        return false;
    }

    void CContext::start()
    {
        if(!_enabled || !_fsm) return;

        _variables = _fsm->getVariables();
        const IState* state = _fsm->getInitialState();

        _timeInState = 0.f;
        _stateFinished = false;

        if (state)
        {
            _currentState = state;
            _currentState->onEnter(*this);
        }

        _active = true;
    }

    void CContext::stop()
    {
        changeState(nullptr);
        _active = false;
    }

    bool CContext::isValid()
    {
        const CFSM* fsm = getFSM();

        for (auto& state : fsm->_states)
        {
            if (!state->checkContext(*this))
                return false;
        }

        return true;
    }

    void CContext::reset()
    {
        stop();

        if (_fsm)
        {
            _variables = _fsm->getVariables();
        }
    }

    void CContext::changeState(const ITransition* transition)
    {
        if (_currentState)
        {
            _currentState->onExit(*this);
            _currentState = nullptr;
        }

        _timeInState = 0.f;
        _stateFinished = false;

        const IState* newState = transition->target;

        if (newState)
        {
            _currentState = newState;
            _currentState->onEnter(*this, transition);
        }
    }

    void CContext::forceState(const std::string& state_name, float time_start)
    {
        if (_currentState)
        {
            _currentState->onExit(*this);
            _currentState = nullptr;
        }

        _timeInState = time_start;
        _stateFinished = false;

        // This will be always a logic state
        CStateBaseLogic* state = (CStateBaseLogic*)_fsm->getState(state_name);

        if (state)
        {
            _currentState = state;
            ITransition transition;
            transition.target = state;

            const TSimpleAnimation& anim = state->getAnimation();
            if (!anim.loop) {
                ((CStateBaseLogic*)_currentState)->getAnimation().back_cycle = "Idle";
            }
            _currentState->onEnter(*this, &transition);
        }
    }

    TVariableValue CContext::getVariableValue(const std::string& name) const
    {
        const auto it = _variables.find(name);
        return it != _variables.cend() ? it->second.getValue() : TVariableValue();
    }

    void CContext::setVariableValue(const std::string& name, const TVariableValue& value)
    {
        const auto it = _variables.find(name);
        if(it != _variables.cend())
        {
            it->second.setValue(value);
        }
    }

    CVariable* CContext::getVariable(const std::string& name)
    {
        const auto it = _variables.find(name);
        return it != _variables.cend() ? &it->second : nullptr;
    }

    void CContext::renderInMenu()
    {
        ImGui::Checkbox("Enabled", &_enabled);

        if (_currentState)
        {
            _currentState->renderInMenu(*this, "Current State: ");
        }
        else
        {
            ImGui::Text("Current state: ...");
        }
        ImGui::Text("Time In State: %.2f", _timeInState);

        if (ImGui::TreeNode("Variables"))
        {
            for (auto& var : _variables)
            {
                var.second.renderInMenu();
            }

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("FSM Resource"))
        {
            if (_fsm)
            {
                _fsm->renderInMenu();
            }
            ImGui::TreePop();
        }
    }
}