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

class CStateParry : public CStateBaseLogic
{
    void onFirstFrameLogic(CContext& ctx) const
    {
        CEntity* owner = ctx.getOwnerEntity();
        static_cast<TCompStamina*>(owner->get<TCompStamina>())->reduceStamina(EAction::PARRY);
    }

public:
    void onLoad(const json& params)
    {
        anim.load(params);
        loadStateProperties(params);
    }

    void onEnter(CContext& ctx, const ITransition* transition) const
    {
        anim.play(ctx, transition);
        onFirstFrameLogic(ctx);
    }

    void onExit(CContext& ctx) const
    {
        ctx.setVariableValue("in_parry", false);
        anim.stop(ctx);
    }

    void onUpdate(CContext& ctx, float dt) const
    {
        anim.update(ctx, dt);
        updateLogic(ctx, dt);
    }
};

REGISTER_STATE("parry", CStateParry)