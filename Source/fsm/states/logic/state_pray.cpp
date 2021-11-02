#include "mcv_platform.h"
#include "engine.h"
#include "entity/entity.h"
#include "fsm/fsm_context.h"
#include "fsm/fsm_parser.h"
#include "fsm/states/logic/state_logic.h"
#include "animation/blend_animation.h"
#include "modules/module_physics.h"
#include "modules/game/module_player_interaction.h"
#include "components/common/comp_transform.h"
#include "components/stats/comp_stamina.h"
#include "components/abilities/comp_time_reversal.h"
#include "components/controllers/comp_player_controller.h"
#include "skeleton/comp_skeleton.h"
#include "skeleton/game_core_skeleton.h"

using namespace fsm;

class CStatePray : public CStateBaseLogic
{
    void onFirstFrameLogic(CContext& ctx) const
    {

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
        CEntity* owner = ctx.getOwnerEntity();

        if (!anim.aborted(ctx)) {
            TCompTransform* transform = owner->get<TCompTransform>();
            TCompCollider* collider = owner->get<TCompCollider>();
            CEntity* e_shrine = PlayerInteraction.getLastShrine();

            TMsgShrinePray msg;
            msg.position = transform->getPosition();
            msg.collider_pos = PXEXVEC3_TO_VEC3(collider->getControllerPosition());
            e_shrine->sendMsg(msg);
            dbg("Prayer done!\n");
        }

        ctx.setVariableValue("is_praying", false);

        TCompTimeReversal* c_time_reversal = owner->get<TCompTimeReversal>();
        c_time_reversal->clearBuffer();

        anim.stop(ctx);
    }

    void onUpdate(CContext& ctx, float dt) const
    {
        anim.update(ctx, dt);
    }
};

REGISTER_STATE("pray", CStatePray)