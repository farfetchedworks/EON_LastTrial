#include "mcv_platform.h"
#include "state_locomotion.h"
#include "components/common/comp_transform.h"
#include "skeleton/comp_skeleton.h"
#include "skeleton/game_core_skeleton.h"
#include "components/controllers/comp_player_controller.h"

using namespace fsm;

void CStateLocomotion::onLoad(const json& params)
{
    IState::setAsBlendState();
    anim.load(params);
    loadStateProperties(params);
}

void CStateLocomotion::onEnter(CContext& ctx, const ITransition* transition) const
{
    anim.play(ctx, transition);
}

void CStateLocomotion::onExit(CContext& ctx) const
{
    CEntity* owner = ctx.getOwnerEntity();
    TCompPlayerController* controller = owner->get<TCompPlayerController>();
    controller->is_sprinting = false;
    anim.stop(ctx);
}

void CStateLocomotion::onUpdate(CContext& ctx, float dt) const
{
    anim.update(ctx, dt);

    CEntity* owner = ctx.getOwnerEntity();
    TCompPlayerController* controller = owner->get<TCompPlayerController>();
    controller->move(dt);
}

void CStateLocomotion::renderInMenu(CContext& ctx, const std::string& prefix) const
{
    IState::renderInMenu(prefix);

    if (anim.isValid)
        anim.renderInMenu(ctx);

    if (ImGui::TreeNode("Mixer")) {

        CEntity* eOwner = ctx.getOwnerEntity();
        if (!eOwner) return;

        TCompSkeleton* c_skel = eOwner->get<TCompSkeleton>();
        CalModel* model = c_skel->model;
        // Dump Mixer
        auto mixer = model->getMixer();
        ImGui::Text("Time: %f/%f", mixer->getAnimationTime(), mixer->getAnimationDuration());
        ImGui::Separator();
        ImGui::Text("ACTIONS");
        for (auto a : mixer->getAnimationActionList()) {
            ImGui::PushID(a);

            std::string ss = "";

            switch (a->getState()) {
            case CalAnimation::State::STATE_NONE: ss += "NONE"; break;
            case CalAnimation::State::STATE_SYNC: ss += "SYNC"; break;
            case CalAnimation::State::STATE_ASYNC: ss += "ASYNC"; break;
            case CalAnimation::State::STATE_IN: ss += "IN"; break;
            case CalAnimation::State::STATE_STEADY: ss += "STEADY"; break;
            case CalAnimation::State::STATE_OUT: ss += "OUT"; break;
            case CalAnimation::State::STATE_STOPPED: ss += "STOPPED"; break;
            }

            ImGui::Text("Action %s S:%s W:%1.2f Time:%1.4f/%1.4f"
                , a->getCoreAnimation()->getName().c_str()
                , ss.c_str()
                , a->getWeight()
                , a->getTime()
                , a->getCoreAnimation()->getDuration()
            );
            ImGui::SameLine();
            if (ImGui::SmallButton("X")) {
                auto core = (CGameCoreSkeleton*)model->getCoreModel();
                int id = core->getCoreAnimationId(a->getCoreAnimation()->getName());
                if (a->getState() == CalAnimation::State::STATE_STOPPED)
                    mixer->removeAction(id, 0.f);
                else
                    a->remove(1.f);
                ImGui::PopID();
                break;
            }
            ImGui::PopID();
        }

        ImGui::Separator();
        ImGui::Text("CYCLES");
        for (auto a : mixer->getAnimationCycle()) {
            ImGui::PushID(a);

            std::string ss = "";

            switch (a->getState()) {
            case CalAnimation::State::STATE_NONE: ss += "NONE"; break;
            case CalAnimation::State::STATE_SYNC: ss += "SYNC"; break;
            case CalAnimation::State::STATE_ASYNC: ss += "ASYNC"; break;
            case CalAnimation::State::STATE_IN: ss += "IN"; break;
            case CalAnimation::State::STATE_STEADY: ss += "STEADY"; break;
            case CalAnimation::State::STATE_OUT: ss += "OUT"; break;
            case CalAnimation::State::STATE_STOPPED: ss += "STOPPED"; break;
            }

            ImGui::Text("Cycle %s S:%s W:%1.2f Time:%1.4f"
                , a->getCoreAnimation()->getName().c_str()
                , ss.c_str()
                , a->getWeight()
                , a->getCoreAnimation()->getDuration()
            );
            ImGui::SameLine();
            if (ImGui::SmallButton("X")) {
                auto core = (CGameCoreSkeleton*)model->getCoreModel();
                int id = core->getCoreAnimationId(a->getCoreAnimation()->getName());
                mixer->clearCycle(id, 1.f);
            }
            ImGui::PopID();
        }

        ImGui::TreePop();
    }
}

REGISTER_STATE("locomotion", CStateLocomotion)