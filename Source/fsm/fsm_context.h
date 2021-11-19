#pragma once

#include "resources/resources.h"
#include "fsm_state.h"
#include "fsm_transition.h"
#include "fsm_variable.h"

namespace FMOD {
    namespace Studio {
        class EventInstance;
    }
}

namespace fsm
{
    class CFSM;

    class CContext : public IResource
    {
    public:
        void update(float dt);

        void setEnabled(bool enabled);
        void setFSM(const CFSM* fsm) { _fsm = fsm; }
        void setOwnerEntity(CHandle owner) { _owner = owner; }
        void setVariableValue(const std::string& name, const TVariableValue& value);
        void setStateFinished(bool finished) { _stateFinished = finished; }
        void setNameVar(std::string name) { _nameVar = name; }
        std::string getNameVar() { return _nameVar; }
        void setFmodEvent(FMOD::Studio::EventInstance* fmod_event) { _cur_fmod_event = fmod_event; }

        void start();
        void stop();
        void reset();

        const CFSM* getFSM() { return _fsm; }
        float getTimeInState() const { return _timeInState; }
        bool isStateFinished() const { return _stateFinished; }
        CHandle getOwnerEntity() const { return _owner; }
        const IState* getCurrentState() const { return _currentState; }
        TVariableValue getVariableValue(const std::string& name) const;
        CVariable* getVariable(const std::string& name);
        void forceState(const std::string& state_name, float time_start);
        bool isValid();
        bool isEnabled() { return _enabled; };

        void renderInMenu();

        // Stored audio events, to kill whenever the state is cancelled (e.g.: animation gets cancelled)
        FMOD::Studio::EventInstance* _cur_fmod_event = nullptr;

    private:
        void changeState(const ITransition* transition);
        bool checkTransitions();

        bool _enabled = true;
        bool _active = false;
        const CFSM* _fsm = nullptr;
        const IState* _currentState = nullptr;
        float _timeInState = 0.f;
        bool _stateFinished = false;
        CHandle _owner;
        MVariables _variables;
        std::string _nameVar = "";
    };
}