#include "mcv_platform.h"
#include "fsm/fsm_transition.h"
#include "fsm/fsm_state.h"

namespace fsm
{
    void ITransition::renderInMenu() const
    {
        ImGui::Text("[%s] %s -> %s", type.data()
            , source ? source->name.c_str() : "..."
            , target ? target->name.c_str() : "...");
    }
}
