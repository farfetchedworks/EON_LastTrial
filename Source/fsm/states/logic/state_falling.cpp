#include "mcv_platform.h"
#include "engine.h"
#include "entity/entity.h"
#include "fsm/fsm_context.h"
#include "fsm/fsm_parser.h"
#include "fsm/states/logic/state_logic.h"
#include "animation/blend_animation.h"
#include "modules/module_physics.h"
#include "ui/ui_module.h"
#include "components/common/comp_transform.h"
#include "components/stats/comp_stamina.h"
#include "components/controllers/comp_player_controller.h"
#include "components/stats/comp_health.h"
#include "skeleton/comp_skeleton.h"
#include "skeleton/game_core_skeleton.h"
#include "input/input_module.h"

using namespace fsm;

class CStateFalling : public CStateBaseLogic
{
    const float maxFallTime = 2.f;

    void onFirstFrameLogic(CContext& ctx) const
    {
        ctx.setVariableValue("fall_time", 0.f);
    }

    void checkFallingCondition(CContext& ctx, float dt) const
    {
        CEntity* owner = ctx.getOwnerEntity();
        TCompPlayerController* controller = owner->get<TCompPlayerController>();
        TCompCollider* collider = owner->get<TCompCollider>();
        TCompTransform* trans = owner->get<TCompTransform>();

        float fallingTime = std::get<float>(ctx.getVariableValue("fall_time"));
        controller->movePhysics(trans->getForward() * 2.f * dt, dt);

        if (!controller->manageFalling(controller->getSpeed(), dt)) {
            
            if (fallingTime > 0.15f) {

                collider->stopFollowForceActor("dash");
                controller->setFalling(false);
                ctx.setVariableValue("is_falling", false);
                
                if (fallingTime > 0.4f) {
                    TCompHealth* health = owner->get<TCompHealth>();
                    int fall_damage = static_cast<int>(health->getMaxHealth() * (fallingTime / maxFallTime));
                    EngineUI.fadeWidget("screen_damage", 2.f);
                    TMsgReduceHealth hmsg;
                    hmsg.damage = fall_damage;
                    hmsg.h_striker = owner;
                    hmsg.skip_blood = true;
                    owner->sendMsg(hmsg);
                }
            }
            else
            {
                // caida corta
                ctx.setVariableValue("is_landing", false);
            }
        }
        else
        {
            fallingTime += dt;
        }

        if (controller->checkDashInput()) {
            controller->setDashAnim();
        }

        ctx.setVariableValue("fall_time", fallingTime);
    }

public:
    void onLoad(const json& params)
    {
        anim.load(params);
    }

    void onEnter(CContext& ctx, const ITransition* transition) const
    {
        anim.play(ctx, transition);
        onFirstFrameLogic(ctx);
    }

    void onExit(CContext& ctx) const
    {
        CEntity* owner = ctx.getOwnerEntity();
        TCompPlayerController* controller = owner->get<TCompPlayerController>();
        controller->setFalling(false);
        ctx.setVariableValue("is_falling", false);
        ctx.setVariableValue("is_landing", true);
        ctx.setVariableValue("fall_time", 0.f);
    }

    void onUpdate(CContext& ctx, float dt) const
    {
        anim.update(ctx, dt);
        checkFallingCondition(ctx, dt);
    }
};

REGISTER_STATE("falling", CStateFalling)