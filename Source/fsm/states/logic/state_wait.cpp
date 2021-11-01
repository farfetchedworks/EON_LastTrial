#include "mcv_platform.h"
#include "engine.h"
#include "entity/entity.h"
#include "fsm/fsm_context.h"
#include "fsm/fsm_parser.h"
#include "state_logic.h"
#include "components/common/comp_transform.h"
#include "components/controllers/comp_player_controller.h"
#include "skeleton/comp_skeleton.h"
#include "skeleton/game_core_skeleton.h"

using namespace fsm;

class CStateWait : public CStateBaseLogic
{
public:
    void onLoad(const json& params)
    {
        loadStateProperties(params);

        callbacks.onRecoveryFinished = [&](CContext& ctx) {
            ctx.setStateFinished(true);
        };
    }

    void onUpdate(CContext& ctx, float dt) const
    {
        updateLogic(ctx, dt);
    }
};

REGISTER_STATE("wait", CStateWait)