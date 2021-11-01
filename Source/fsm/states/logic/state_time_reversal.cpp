#include "mcv_platform.h"
#include "engine.h"
#include "entity/entity.h"
#include "fsm/fsm_context.h"
#include "fsm/fsm_parser.h"
#include "fsm/states/logic/state_logic.h"
#include "animation/blend_animation.h"
#include "modules/module_physics.h"
#include "input/input_module.h"
#include "components/common/comp_transform.h"
#include "components/abilities/comp_time_reversal.h"
#include "components/controllers/comp_player_controller.h"
#include "skeleton/comp_skeleton.h"
#include "skeleton/game_core_skeleton.h"

using namespace fsm;

class CStateTimeReversal : public CStateBaseLogic
{
public:
    
    void onLoad(const json& params)
    {
        // load cancel state
        loadStateProperties(params);
    }

    void onEnter(CContext& ctx, const ITransition* transition) const
    {
        if (!std::get<bool>(ctx.getVariableValue("is_death")))
            return;

        CEntity* e_gameManager = getEntityByName("game_manager");
        TMsgEonHasRevived msgEonRevived;
        e_gameManager->sendMsg(msgEonRevived);
        ctx.setVariableValue("is_death", false);
    }

    void onUpdate(CContext& ctx, float dt) const
    {
        CEntity* owner = ctx.getOwnerEntity();
        TCompTimeReversal* timeReversal = owner->get<TCompTimeReversal>();

        // After players stops rewinding releasing key
        if (PlayerInput["time_reversal"].getsReleased()) {
            timeReversal->stopRewinding();
            ctx.setVariableValue("in_time_reversal", false);
        }
    }
};

REGISTER_STATE("timereversal", CStateTimeReversal)