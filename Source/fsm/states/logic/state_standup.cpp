#include "mcv_platform.h"
#include "engine.h"
#include "entity/entity.h"
#include "fsm/fsm_context.h"
#include "fsm/fsm_parser.h"
#include "fsm/states/logic/state_logic.h"
#include "animation/blend_animation.h"
#include "modules/module_physics.h"
#include "components/common/comp_transform.h"
#include "components/stats/comp_stamina.h"
#include "components/controllers/comp_player_controller.h"
#include "skeleton/comp_skeleton.h"
#include "skeleton/game_core_skeleton.h"

using namespace fsm;

class CStateStandUp : public CStateBaseLogic
{
public:
    void onLoad(const json& params)
    {
        anim.load(params);
        loadStateProperties(params);

        callbacks.onRecovery = [&](CContext& ctx) {
            CEntity* owner = ctx.getOwnerEntity();
            TCompPlayerController* controller = owner->get<TCompPlayerController>();

            if (controller->checkDashInput()) {
                controller->calcMoveDirection();
                ctx.setVariableValue("is_dashing", true);
                ctx.setStateFinished(true);
            }

            if (controller->isMoving()) {
                ctx.setStateFinished(true);
            }
        };
    }

    void onEnter(CContext& ctx, const ITransition* transition) const
    {
        anim.play(ctx, transition);
    }

    void onUpdate(CContext& ctx, float dt) const
    {
        anim.update(ctx, dt);
        updateLogic(ctx, dt);
    }

    void onExit(CContext& ctx) const
    {
        anim.stop(ctx);
    }
};

REGISTER_STATE("standup", CStateStandUp)