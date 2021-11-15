#include "mcv_platform.h"
#include "fsm/fsm.h"
#include "fsm/fsm_context.h"
#include "entity/entity.h"
#include "state_logic.h"
#include "components/common/comp_transform.h"
#include "components/messages.h"
#include "skeleton/comp_skeleton.h"
#include "skeleton/game_core_skeleton.h"

namespace fsm
{
    const float framerate = 30.f;

    void CStateBaseLogic::loadStateProperties(const json& params)
    {
        const std::string& stateName = params["name"];

        if (params.count("timings")) {
            
            bool is_ok = loadStateTimings(params);
            assert(is_ok);
        }
        
        if(params.count("cancel")) {
            const json& jCancelMode = params["cancel"];
            assert(jCancelMode.is_string());
            const std::string& sTimings = jCancelMode;

            if (sTimings == "default") cancel_mode = EStateCancel::NOT_CANCELLABLE;
            else if (sTimings == "on_hit") cancel_mode = EStateCancel::ON_HIT;
            else if (sTimings == "on_decision") cancel_mode = EStateCancel::ON_DECISION;
            else if (sTimings == "on_any") cancel_mode = EStateCancel::ON_ANY;
            else fatal("Invalid cancel mode in state '%s'\n", stateName.c_str());
        }
    }

    bool CStateBaseLogic::loadStateTimings(const json& params)
    {
        const std::string& stateName = params["name"];
        const json& jTimings = params["timings"];
        assert(jTimings.is_string());
        const std::string& sTimings = jTimings;

        VEC3 v;
        int n = sscanf(sTimings.c_str(), "%f %f %f", &v.x, &v.y, &v.z);
        if (n != 3)
            return false;

        bool inFrames = true;

        if (inFrames) {
            // Frames don't use the fraction part
            v.Floor(); 
            v /= framerate;
        }

        timings.fill(v);
        use_timings = true;
        return true;
    }

    bool CStateBaseLogic::checkContext(CContext& ctx) const
    {
        // "Wait" doesn't need animation to use timings
        if (name == "Wait")
            return true;

        // Check if timings are correct for current animation
        if (use_timings && anim.isValid && !anim.loop)
        {
            float timings_duration = timings.getTime();
            float anim_duration = anim.getAnimationTime(ctx);

            // skip that.. for sure it's on purpose
            if (timings_duration * framerate > 500.f)
            {
                return true;
            }

            if (fabs(timings_duration - anim_duration) > 0.05f) {
                fatal("%s: Timings time (%f) of state [%s] don't match animation [%s] duration time (%f)", 
                    ctx.getFSM()->getName().c_str(),
                    timings.getTime(), name.c_str(),
                    anim.name.c_str(), anim_duration);
                return false;
            }
        }

        return true;
    }

    void CStateBaseLogic::renderInMenu(CContext& ctx, const std::string& prefix) const
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
}
