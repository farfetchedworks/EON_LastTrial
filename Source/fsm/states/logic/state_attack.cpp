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
#include "input/input_module.h"

using namespace fsm;

class CStateAttack : public CStateBaseLogic
{
    EAction getCurrentAction(CContext& ctx) const
    {
        CEntity* owner = ctx.getOwnerEntity();
        TCompPlayerController* controller = owner->get<TCompPlayerController>();
        EAction action = EAction::ATTACK_REGULAR;
        if (controller->attack_count > 1) // 2 or 3 by now
            action = EAction::ATTACK_REGULAR_COMBO;
        else if (std::get<bool>(ctx.getVariableValue("is_sprint_regular_attack")))
            action = EAction::ATTACK_REGULAR_SPRINT;

        return action;
    }

protected:

    void onFirstFrameLogic(CContext& ctx) const
    {
        CEntity* owner = ctx.getOwnerEntity();
        TCompTransform* transform = owner->get<TCompTransform>();
        TCompPlayerController* controller = owner->get<TCompPlayerController>();

        controller->continue_attacking = false;
        controller->continue_attacking2 = false;
        controller->can_recover_stamina = false;

        controller->resetMoveTimer();

        // only scenario collisions in startup should stun player
        controller->setWeaponPartStatus(true, controller->attack_count);

        VEC3 origin = transform->getPosition();// +transform->getForward();
        VHandles colliders;
        bool hasOverlapped = EnginePhysics.overlapSphere(origin, controller->getAttackRange(), colliders, CModulePhysics::FilterGroup::Enemy);
        if (hasOverlapped) {
            for (auto other : colliders) {
                notifyEnemy(ctx.getOwnerEntity(), other.getOwner());
            }
        }
    }

    void notifyEnemy(CHandle h_owner, CEntity* enemy) const
    {
        // Enemy can be null if killed in this frame
        if (!enemy) {
            return;
        }

        assert(h_owner.isValid());

        {
            TMsgPreHit msgPreHit;
            msgPreHit.hitByPlayer = true;
            msgPreHit.h_striker = h_owner;
            enemy->sendMsg(msgPreHit);
        }
    }

    void resetAttackVariables(CContext& ctx) const
    {
        ctx.setVariableValue("attack_regular", 0);
        ctx.setVariableValue("is_attacking_heavy", 0);
        ctx.setVariableValue("is_dash_strike", false);
        ctx.setVariableValue("is_sprint_regular_attack", false);
        ctx.setVariableValue("is_sprint_strong_attack", false);
    }

public:
    void onLoad(const json& params)
    {
        anim.load(params);
        loadStateProperties(params);

        callbacks.onStartup = [&](CContext& ctx) {
            CEntity* owner = ctx.getOwnerEntity();
            TCompPlayerController* controller = owner->get<TCompPlayerController>();

            VEC3 curr_dir = controller->getMoveDirection(controller->is_moving);
            // change attack direction
            if (!controller->is_locked_on && curr_dir != VEC3::Zero) {
                // save the last direction
                controller->last_dir = curr_dir;
            }
        };

        callbacks.onStartupFinished = [&](CContext& ctx) {
            CEntity* owner = ctx.getOwnerEntity();
            TCompPlayerController* controller = owner->get<TCompPlayerController>();
            controller->setWeaponStatus(true, getCurrentAction(ctx));
            static_cast<TCompStamina*>(owner->get<TCompStamina>())->reduceStamina(getCurrentAction(ctx));
        };

        callbacks.onActive = [&](CContext& ctx) {

            CEntity* owner = ctx.getOwnerEntity();
            TCompTransform* transform = owner->get<TCompTransform>();
            TCompPlayerController* controller = owner->get<TCompPlayerController>();

            bool is_dash_strike = std::get<bool>(ctx.getVariableValue("is_dash_strike"));
            if (PlayerInput["attack_regular"].getsPressed() && controller->hasStamina() && !is_dash_strike) {
                controller->continue_attacking = true;
            }

            if (PlayerInput["attack_heavy"].getsPressed() && controller->hasStamina() && !is_dash_strike) {
                controller->continue_attacking2 = true;
            }

            if (!controller->is_locked_on) {
                // to complete the rotation
                QUAT current_rot = transform->getRotation();
                QUAT new_rot = QUAT::CreateFromAxisAngle(VEC3(0, 1, 0), atan2(controller->last_dir.x, controller->last_dir.z));
                transform->setRotation(dampQUAT(current_rot, new_rot, 1.0f, Time.delta));
            }
        };

        // Set callbacks
        callbacks.onActiveFinished = [&](CContext& ctx) {

            CEntity* owner = ctx.getOwnerEntity();
            TCompTransform* transform = owner->get<TCompTransform>();
            TCompPlayerController* controller = owner->get<TCompPlayerController>();
            controller->setWeaponStatus(false);
            controller->setWeaponPartStatus(false);

            // If the player continues attacking during active frames, get next attack
            if (controller->continue_attacking) {

                bool sprintRegular = std::get<bool>(ctx.getVariableValue("is_sprint_regular_attack"));
                bool sprintHeavy = std::get<bool>(ctx.getVariableValue("is_sprint_strong_attack"));
                int heavyAttack = std::get<int>(ctx.getVariableValue("is_attacking_heavy"));
                int nextAttack = controller->getNextAttack();

                // connect to combo attack (2), then the combo connects with its own connections
                if (heavyAttack == 2) {
                    nextAttack = controller->attack_count = 2;
                }
                else if (sprintRegular || sprintHeavy) {
                    nextAttack = controller->attack_count = 4;
                }

                if (nextAttack > 0) {
                    resetAttackVariables(ctx);
                    ctx.setVariableValue("attack_regular", nextAttack);
                }
            }
            else if (controller->continue_attacking2) {

                int attackRegular = std::get<int>(ctx.getVariableValue("attack_regular"));
                bool sprintHeavy = std::get<bool>(ctx.getVariableValue("is_sprint_strong_attack"));
                int heavyAttack = std::get<int>(ctx.getVariableValue("is_attacking_heavy"));

                if (!sprintHeavy && heavyAttack == 0) {
                    resetAttackVariables(ctx);

                    if(attackRegular == 2)
                        ctx.setVariableValue("is_attacking_heavy", 2);
                }
            }
            else {
                resetAttackVariables(ctx);
            }
        };

        callbacks.onRecovery = [&](CContext& ctx) {

            CEntity* owner = ctx.getOwnerEntity();
            TCompPlayerController* controller = owner->get<TCompPlayerController>();
            EAction currentAction = getCurrentAction(ctx);

            if (controller->checkDashInput()) {
                controller->calcMoveDirection();
                controller->resetAttackCombo();
                resetAttackVariables(ctx);
                ctx.setVariableValue("is_dashing", true);
                ctx.setStateFinished(true);
            }

            // If the player is moving, immediately blend to the locomotion animation and cancel the attack recovery
            // Don't do this when doing specific attacks as sprint attacks or heavy attacks
            if (controller->isMoving(true) && !controller->continue_attacking && !controller->continue_attacking2) {
                resetAttackVariables(ctx);
                controller->resetAttackCombo();
                ctx.setStateFinished(true);
            }

            // Don't do nothing i already attacked on active
            if (controller->continue_attacking || controller->continue_attacking2)
                return;

            bool is_dash_strike = std::get<bool>(ctx.getVariableValue("is_dash_strike"));
            if (PlayerInput["attack_regular"].getsPressed() && controller->hasStamina() && !is_dash_strike) {
                int nextAttack = controller->getNextAttack(true);
                ctx.setVariableValue("attack_regular", nextAttack);
            }

            if (PlayerInput["attack_heavy"].getsPressed() && controller->hasStamina() && !is_dash_strike) {
                if(controller->attack_count == 2)
                    ctx.setVariableValue("is_attacking_heavy", 2);
            }

        };
    }

    void onEnter(CContext& ctx, const ITransition* transition) const
    {
        bool dashStrike = std::get<bool>(ctx.getVariableValue("is_dash_strike"));
        if (dashStrike)
        {
            anim.start_time = std::get<float>(ctx.getVariableValue("last_time_in_state"));
        }

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
        int attackRegular = std::get<int>(ctx.getVariableValue("attack_regular"));
        int attackHeavy = std::get<int>(ctx.getVariableValue("is_attacking_heavy"));

        CEntity* owner = ctx.getOwnerEntity();
        TCompPlayerController* controller = owner->get<TCompPlayerController>();

        resetAttackVariables(ctx);
        controller->resetAttackCombo();
        controller->is_sprint_attack = false;
        controller->is_dash_strike = false;

        // Combo not finished
        if (attackRegular != 0) {
            controller->attack_count = attackRegular;
            ctx.setVariableValue("attack_regular", attackRegular);
        }
        else if (attackHeavy != 0) {
            ctx.setVariableValue("is_attacking_heavy", attackHeavy);
        }

        anim.stop(ctx);

        // Attack interrupted
        bool stunned = std::get<bool>(ctx.getVariableValue("is_stunned"));
        if (stunned) {
            resetAttackVariables(ctx);
            controller->resetAttackCombo();
            controller->setWeaponStatus(false);
            controller->setWeaponPartStatus(false);
        }
    }
};

REGISTER_STATE("attack", CStateAttack)

/* -----------------------------------------------------------
    We can make the heavy to inherit the regular since almost 
    everything is shared
*/ 

class CStateAttackHeavy : public CStateAttack
{
    EAction getCurrentAction(CContext& ctx) const
    {
        EAction action = EAction::ATTACK_HEAVY;
        if (std::get<int>(ctx.getVariableValue("is_attacking_heavy")) == 2)
            action = EAction::ATTACK_HEAVY_CHARGE;
        else if (std::get<bool>(ctx.getVariableValue("is_sprint_strong_attack")))
            action = EAction::ATTACK_HEAVY_SPRINT;

        return action;
    }

    void onFirstFrameLogic(CContext& ctx) const
    {
        CEntity* owner = ctx.getOwnerEntity();
        TCompTransform* transform = owner->get<TCompTransform>();
        TCompPlayerController* controller = owner->get<TCompPlayerController>();
        controller->can_recover_stamina = false;
        controller->resetMoveTimer();

        VEC3 origin = transform->getPosition();// +transform->getForward();
        VHandles colliders;
        bool hasOverlapped = EnginePhysics.overlapSphere(origin, controller->getAttackRange(), colliders, CModulePhysics::FilterGroup::Enemy);
        if (hasOverlapped) {
            for (auto other : colliders) {
                notifyEnemy(ctx.getOwnerEntity(), other.getOwner());
            }
        }
    }

public:

    void onLoad(const json& params)
    {
        CStateAttack::onLoad(params);

        callbacks.onStartupFinished = [&](CContext& ctx) {
            CEntity* owner = ctx.getOwnerEntity();
            TCompPlayerController* controller = owner->get<TCompPlayerController>();
            controller->setWeaponStatus(true, getCurrentAction(ctx));
            static_cast<TCompStamina*>(owner->get<TCompStamina>())->reduceStamina(getCurrentAction(ctx));
        };

        callbacks.onRecovery = [&](CContext& ctx) {

            CEntity* owner = ctx.getOwnerEntity();
            TCompPlayerController* controller = owner->get<TCompPlayerController>();
            EAction currentAction = getCurrentAction(ctx);

            if (controller->checkDashInput()) {
                controller->calcMoveDirection();
                controller->resetAttackCombo();
                resetAttackVariables(ctx);
                ctx.setVariableValue("is_dashing", true);
                ctx.setStateFinished(true);
            }
        };
    }
};

REGISTER_STATE("heavyattack", CStateAttackHeavy)