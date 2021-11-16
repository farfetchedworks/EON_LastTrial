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
    float distanceToGround = 0.0f;

    void onFirstFrameLogic(CContext& ctx) const
    {
        CEntity* owner = ctx.getOwnerEntity();
        TCompCollider* collider = owner->get<TCompCollider>();
        TCompPlayerController* controller = owner->get<TCompPlayerController>();
        controller->distance_to_ground = collider->distanceToGround();
    }

    void checkFallingCondition(CContext& ctx, float dt) const
    {
        CEntity* owner = ctx.getOwnerEntity();
        TCompPlayerController* controller = owner->get<TCompPlayerController>();
        TCompCollider* collider = owner->get<TCompCollider>();

        bool is_moving = false;
        //controller->move_dir = controller->getMoveDirection(is_moving);

        // To apply gravity
        controller->movePhysics(VEC3::Zero, dt);

        float current_speed = controller->getSpeed();

        // TODO Refactoring
        if (!controller->manageFalling(current_speed, Time.delta)) {
            
            float distance = controller->distance_to_ground;
            if (distance > 0.5f) {

                collider->stopFollowForceActor("dash");
                controller->setFalling(false);
                ctx.setVariableValue("is_falling", false);
                
                if (distance > 3.0f) {
                    TCompHealth* health = owner->get<TCompHealth>();
                    // fall damage
                    float max_distance = 20.0f;
                    distance = std::min<float>(distance, max_distance);
                    int fall_damage = static_cast<int>(health->getMaxHealth() * (distance / max_distance));
                    
                    {
                        EngineUI.fadeWidget("screen_damage", 2.f);

                        // Reduce health
                        TMsgReduceHealth hmsg;
                        hmsg.damage = fall_damage;
                        hmsg.h_striker = owner;
                        hmsg.hitByPlayer = false;
                        hmsg.fromBack = false;
                        owner->sendMsg(hmsg);
                    }
                }
            }
            else
            {
                // caida corta
                ctx.setVariableValue("is_landing", false);
            }
        }

        if (controller->checkDashInput()) {
            controller->setDashAnim();
        }
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
    }

    void onUpdate(CContext& ctx, float dt) const
    {
        anim.update(ctx, dt);
        checkFallingCondition(ctx, dt);
        CEntity* owner = ctx.getOwnerEntity();
        TCompCollider* collider = owner->get<TCompCollider>();
        //dbg("%f\n", PXVEC3_TO_VEC3(((physx::PxRigidDynamic*)collider->actor)->getLinearVelocity()).y);
    }
};

REGISTER_STATE("falling", CStateFalling)