#include "mcv_platform.h"
#include "fsm/fsm_state.h"
#include "fsm/fsm_transition.h"

namespace fsm
{
    void IState::renderInMenu(CContext& ctx, const std::string& prefix) const
    {
        renderInMenu(prefix);
    }

    void IState::renderInMenu(const std::string& prefix) const
    {
        if (ImGui::TreeNode(this, "%s[%s] %s", prefix.c_str(), type.data(), name.c_str()))
        {
            for (auto& tr : transitions)
            {
                tr->renderInMenu();
            }
            ImGui::TreePop();
        }
    }
}
