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

class CStateHitStun : public CStateBaseLogic
{
public:
    void onLoad(const json& params)
    {
        anim.load(params);
    }

    void onEnter(CContext& ctx, const ITransition* transition) const
    {
        anim.play(ctx, transition);
    }

    void onExit(CContext& ctx) const
    {
        anim.stop(ctx);
        CEntity* owner = ctx.getOwnerEntity();
        TCompPlayerController* controller = owner->get<TCompPlayerController>();
        // recover stamina after hitstun and reset combo
        controller->resetAttackCombo();
    }

    void onUpdate(CContext& ctx, float dt) const
    {
        /*
            This has wait_time transition, so we only
            wait that time and onExit will be called
        */
        anim.update(ctx, dt);
    }
};

REGISTER_STATE("hitstun", CStateHitStun)