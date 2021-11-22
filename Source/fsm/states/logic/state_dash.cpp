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
#include "input/input_module.h"
#include "entity/entity_parser.h"

using namespace fsm;

class CStateDash : public CStateBaseLogic
{
public:
    void onFirstFrameLogic(CContext& ctx) const
    {
        CEntity* owner = ctx.getOwnerEntity();
        TCompTransform* transform = owner->get<TCompTransform>();
        TCompPlayerController* controller = owner->get<TCompPlayerController>();

        // Disable stamina gain during its duration
        controller->can_recover_stamina = false;
        bool is_moving = controller->move_dir.LengthSquared() != 0;

        // Player must face dash direction
        if (is_moving) {
            transform->setRotation(QUAT::CreateFromYawPitchRoll(vectorToYaw(controller->move_dir), 0, 0));
        }

        // Set player dash status
        controller->is_dashing = true;
        static_cast<TCompStamina*>(owner->get<TCompStamina>())->reduceStamina(EAction::DASH);
        spawnParticles("data/particles/compute_dash_smoke_particles.json", transform->getPosition() + transform->getForward() * 0.1f, transform->getPosition());
    }

    void enableStaminaGain(CContext& ctx, bool val) const
    {
        CEntity* owner = ctx.getOwnerEntity();
        TCompPlayerController* controller = owner->get<TCompPlayerController>();
        controller->can_recover_stamina = val;
    }

    void onLoad(const json& params)
    {
        anim.load(params);
        loadStateProperties(params);

        callbacks.onStartup = [&](CContext& ctx)
        {
            if (PlayerInput["attack_regular"].getsPressed())
            {
                ctx.setVariableValue("is_dash_strike", true);
                ctx.setStateFinished(true);
            }
        };

        callbacks.onActive = [&](CContext& ctx)
        {
            CEntity* owner = ctx.getOwnerEntity();
            TCompPlayerController* controller = owner->get<TCompPlayerController>();
            controller->movePhysics(VEC3::Down * 0.001f, Time.delta, -1.f);
        };

        callbacks.onRecovery = [&](CContext& ctx)
        {
            CEntity* owner = ctx.getOwnerEntity();
            TCompCollider* collider = owner->get<TCompCollider>();
            TCompTransform* transform = owner->get<TCompTransform>();
            TCompPlayerController* controller = owner->get<TCompPlayerController>();
            controller->movePhysics(VEC3::Down * 0.001f, Time.delta, -1.f);

            float current_speed = controller->getSpeed();

           if (!controller->isFalling() && controller->manageFalling(current_speed, Time.delta)) {
                // controller->setFalling(true);
                ctx.setVariableValue("is_falling", true);
                ctx.setStateFinished(true);
                // add force to end of dash
                VEC3 dir = transform->getForward() - transform->getUp();
                collider->addForce( normVEC3(dir) * 6.f, "dash");
            } 
            //// Alex: No entiendo esto
            else {
                ctx.setVariableValue("is_falling", false);
                ctx.setStateFinished(true);
            }

            if (controller->isMoving()) {
                ctx.setVariableValue("is_falling", false);
                ctx.setStateFinished(true);
            }
        };

        callbacks.onRecoveryFinished = [&](CContext& ctx)
        {
            CEntity* owner = ctx.getOwnerEntity();
            TCompTransform* transform = owner->get<TCompTransform>();
            TCompCollider* collider = owner->get<TCompCollider>();

            VEC3 dir = transform->getForward() - transform->getUp();
            collider->addForce(normVEC3(dir) * 6.f, "dash");
            ctx.setVariableValue("is_dashing", false);
            
        };
    }

    void onEnter(CContext& ctx, const ITransition* transition) const
    {
        anim.play(ctx, transition);
        onFirstFrameLogic(ctx);
    }

    void onUpdate(CContext& ctx, float dt) const
    {
        anim.update(ctx, dt);
        updateLogic(ctx, dt);
    }

    void onExit(CContext& ctx) const
    {
        anim.stop(ctx);

        CEntity* owner = ctx.getOwnerEntity();
        TCompTransform* transform = owner->get<TCompTransform>();
        TCompPlayerController* controller = owner->get<TCompPlayerController>();
        controller->is_dashing = false;

        enableStaminaGain(ctx, true);

        // this is stored to use in the dash strike, to avoid repeating the 
        // first part of the animation and connect them
        ctx.setVariableValue("last_time_in_state", ctx.getTimeInState());
        ctx.setVariableValue("is_dashing", false);
    }
};

REGISTER_STATE("dash", CStateDash)

class CStateDashLockOn : public CStateDash
{
public:
    void onFirstFrameLogic(CContext& ctx) const
    {
        CEntity* owner = ctx.getOwnerEntity();
        TCompTransform* transform = owner->get<TCompTransform>();
        TCompPawnController* pawn_controller = owner->get<TCompPawnController>();
        TCompPlayerController* controller = owner->get<TCompPlayerController>();
        TCompCollider* collider = owner->get<TCompCollider>();

        bool is_moving = controller->move_dir.LengthSquared() != 0;
        controller->dash_dir = is_moving ? controller->move_dir : transform->getForward();
        controller->dash_force = 12.5f;

        // Set player dash status
        controller->is_dashing = true;

        VEC3 force = controller->getDashForce(pawn_controller->speed_multiplier);
        controller->can_enter_area_delay = pawn_controller->speed_multiplier == 1.f;
        controller->can_exit_area_delay = pawn_controller->speed_multiplier != 1.f;

        if (collider->force_actor != nullptr)
        {
            if (ctx.getTimeInState() == 0.f) {
                controller->is_moving = true;
                collider->addForce(force, "dash", true);
            }
        }

        // only scenario collisions in startup should stun player
        controller->setWeaponPartStatus(true, controller->attack_count);

        spawnParticles("data/particles/compute_dash_smoke_particles.json", transform->getPosition() + transform->getForward() * 0.1f, transform->getPosition());
        static_cast<TCompStamina*>(owner->get<TCompStamina>())->reduceStamina(EAction::DASH);
    }

    void onCollisionAreaDelay(CContext& ctx)
    {
        CEntity* owner = ctx.getOwnerEntity();
        TCompPawnController* pawn_controller = owner->get<TCompPawnController>();
        TCompPlayerController* controller = owner->get<TCompPlayerController>();
        TCompCollider* collider = owner->get<TCompCollider>();

        if (controller->can_enter_area_delay && pawn_controller->speed_multiplier != 1.f) {
            controller->can_enter_area_delay = false;
            VEC3 new_force = collider->getLinearVelocity() * pawn_controller->speed_multiplier;
            collider->stopFollowForceActor("dash");
            collider->addForce(new_force, "dash", true);
        }

        if (controller->can_exit_area_delay && pawn_controller->speed_multiplier == 1.f) {
            controller->can_exit_area_delay = false;
            VEC3 new_force = controller->dash_dir * controller->dash_force;
            collider->stopFollowForceActor("dash");
            collider->addForce(new_force, "dash", true);
        }
    }

    void onLoad(const json& params)
    {
        anim.load(params);
        loadStateProperties(params);

        callbacks.onStartup = [&](CContext& ctx)
        {
            CEntity* owner = ctx.getOwnerEntity();
            TCompTransform* transform = owner->get<TCompTransform>();
            TCompCollider* collider = owner->get<TCompCollider>();
            TCompPlayerController* controller = owner->get<TCompPlayerController>();
            onCollisionAreaDelay(ctx);

            if (PlayerInput["attack_regular"].getsPressed() && ctx.getNameVar() == "dash_vertical")
            {
                controller->is_dash_strike = true;
                ctx.setVariableValue("is_dash_strike", true);
                ctx.setVariableValue("last_time_in_state", ctx.getTimeInState());
                ctx.setStateFinished(true);
            }
        };

        callbacks.onActive = [&](CContext& ctx)
        {
            onCollisionAreaDelay(ctx);

            if (ctx.getNameVar() == "dash_vertical")
                return;

            CEntity* owner = ctx.getOwnerEntity();
            TCompPlayerController* controller = owner->get<TCompPlayerController>();
            TCompTransform* transform = owner->get<TCompTransform>();

            // TODO: orbit around enemy when dashing?
            TCompTransform* c_locked_trans = controller->h_locked_transform;
            QUAT current_rot = transform->getRotation();

            VEC3 front;
            if (c_locked_trans) {
                // Face locked enemy
                front = c_locked_trans->getPosition() - transform->getPosition();
                front.Normalize();
            } else {
                front = transform->getForward();
            }

            QUAT new_rot = QUAT::CreateFromAxisAngle(VEC3(0, 1, 0), atan2(front.x, front.z));
            transform->setRotation(dampQUAT(current_rot, new_rot, 10.0f, Time.delta));
        };

        callbacks.onRecovery = [&](CContext& ctx)
        {
            CEntity* owner = ctx.getOwnerEntity();
            TCompTransform* transform = owner->get<TCompTransform>();
            TCompPlayerController* controller = owner->get<TCompPlayerController>();

            if (controller->isMoving()) {
                ctx.setVariableValue("is_falling", false);
                ctx.setStateFinished(true);
            }
            
        };

        callbacks.onRecoveryFinished = [&](CContext& ctx)
        {
            CEntity* owner = ctx.getOwnerEntity();
            TCompPlayerController* controller = owner->get<TCompPlayerController>();
            TCompCollider* collider = owner->get<TCompCollider>();
            TCompTransform* transform = owner->get<TCompTransform>();  
            collider->stopFollowForceActor("dash");
            controller->is_dashing = false;
            ctx.setVariableValue(ctx.getNameVar(), 0.f);
            controller->setWeaponPartStatus(false);
            //spawn("data/particles/dash_floor_particles.json", *transform);
        };
    }

    void onEnter(CContext& ctx, const ITransition* transition) const
    {
        anim.play(ctx, transition);
        onFirstFrameLogic(ctx);
    }

    void onUpdate(CContext& ctx, float dt) const
    {
        CEntity* owner = ctx.getOwnerEntity();
        TCompTransform* transform = owner->get<TCompTransform>();

        anim.update(ctx, dt);
        updateLogic(ctx, dt);
        //spawn("data/particles/dash_floor_particles.json", *transform);
    }

    void onExit(CContext& ctx) const
    {
        CEntity* owner = ctx.getOwnerEntity();
        TCompPlayerController* controller = owner->get<TCompPlayerController>();
        TCompCollider* collider = owner->get<TCompCollider>();
        TCompTransform* transform = owner->get<TCompTransform>();

        // Set player dash status
        controller->is_dashing = false;

        // Stop all forces applied
        collider->stopFollowForceActor("dash");

        float current_speed = controller->getSpeed();
        if (collider->distanceToGround() >= 0.2) {
            // add force to end of dash
            VEC3 dir = controller->dash_dir - transform->getUp();
            collider->addForce(normVEC3(dir) * 6.f, "dash");
        }
        //else {
        //    collider->stopFollowForceActor();
        //}

        ctx.setVariableValue(ctx.getNameVar(), 0.f);
    }
};

class CStateDashForward : public CStateDashLockOn
{
public:

    void onEnter(CContext& ctx, const ITransition* transition) const
    {
        ctx.setNameVar("dash_vertical");
        CStateDash::onEnter(ctx, transition);
        enableStaminaGain(ctx, false);
    }

    void onExit(CContext& ctx) const
    {
        enableStaminaGain(ctx, true);
        ctx.setVariableValue(ctx.getNameVar(), 0.f);
        CStateDash::onExit(ctx);
    }

    void onUpdate(CContext& ctx, float dt) const
    {
        CStateDash::onUpdate(ctx, dt);
    }
};

REGISTER_STATE("dashforward", CStateDashForward)

class CStateDashBackward : public CStateDashLockOn
{
public:

    void onEnter(CContext& ctx, const ITransition* transition) const
    {
        ctx.setNameVar("dash_vertical");
        CStateDashLockOn::onEnter(ctx, transition);
        enableStaminaGain(ctx, false);
    }

    void onExit(CContext& ctx) const
    {
        enableStaminaGain(ctx, true);
        CStateDashLockOn::onExit(ctx);
    }
};

REGISTER_STATE("dashbackward", CStateDashBackward)

class CStateDashLeft : public CStateDashLockOn
{
public:

    void onEnter(CContext& ctx, const ITransition* transition) const
    {
        ctx.setNameVar("dash_horizontal");
        CStateDashLockOn::onEnter(ctx, transition);
        enableStaminaGain(ctx, false);
    }

    void onExit(CContext& ctx) const
    {
        enableStaminaGain(ctx, true);
        CStateDashLockOn::onExit(ctx);
    }
};

REGISTER_STATE("dashleft", CStateDashLeft)

class CStateDashRight : public CStateDashLockOn
{
public:

    void onEnter(CContext& ctx, const ITransition* transition) const
    {
        ctx.setNameVar("dash_horizontal");
        CStateDashLockOn::onEnter(ctx, transition);
        enableStaminaGain(ctx, false);
    }

    void onExit(CContext& ctx) const
    {
        enableStaminaGain(ctx, true);
        CStateDashLockOn::onExit(ctx);
    }
};

REGISTER_STATE("dashright", CStateDashRight)