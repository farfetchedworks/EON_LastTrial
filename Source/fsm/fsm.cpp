#include "mcv_platform.h"
#include "fsm/fsm.h"
#include "fsm/fsm_parser.h"
#include "fsm/fsm_state.h"
#include "fsm/fsm_transition.h"
#include "entity/entity.h"
#include "components/common/comp_fsm.h"

class CFSMResourceType : public CResourceType {
public:
    const char* getExtension(int idx) const { return ".fsm"; }
    const char* getName() const { return "FSM"; }
    IResource* create(const std::string& name) const
    {
        json jData = loadJson(name);
        if (jData.empty())
        {
            return nullptr;
        }

        fsm::CFSM* fsm = new fsm::CFSM();
        const bool ok = fsm::CParser::parse(fsm, jData);
        assert(ok);
        if (!ok)
        {
            delete fsm;
            return nullptr;
        }

        return fsm;
    }
};

template<>
CResourceType* getClassResourceType<fsm::CFSM>() {
    static CFSMResourceType factory;
    return &factory;
}
// -----------------------------------------

namespace fsm
{
    const IState* CFSM::getState(const std::string& name) const
    {
        for (const IState* state : _states)
        {
            if (state->name == name)
            {
                return state;
            }
        }
        return nullptr;
    }

    const ITransition* CFSM::getTransition(const std::string& sourceName, const std::string& targetName) const
    {
        for (const ITransition* transition : _transitions)
        {
            if (transition->source->name == sourceName && transition->target->name == targetName)
            {
                return transition;
            }
        }
        return nullptr;
    }

    bool CFSM::renderInMenu() const
    {
        if (_initialState)
        {
            ImGui::Text("Start: [%s] %s", _initialState->type.data(), _initialState->name.c_str());
        }
        else
        {
            ImGui::Text("Start: ..");
        }

        if (ImGui::TreeNode("States"))
        {
            for (auto& state : _states)
            {
                state->renderInMenu();
            }

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Transitions"))
        {
            for (auto& tr : _transitions)
            {
                tr->renderInMenu();
            }

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Variables"))
        {
            for (auto& var : _variables)
            {
                var.second.renderInMenu();
            }

            ImGui::TreePop();
        }

        return true;
    }

    void CFSM::onFileChange(const std::string& filename)
    {
        if (getName() != filename) return;

        json jData = loadJson(filename);
        if (jData.empty()) return;

        const bool ok = fsm::CParser::parse(this, jData);

        getObjectManager<TCompFSM>()->forEach([&](TCompFSM* comp) {
            if (this == comp->getCtx().getFSM()) {
                if (comp->getCtx().getCurrentState())
                    comp->getCtx().getCurrentState()->onEnter(comp->getCtx());
            }
        });

        assert(ok);
    }

}