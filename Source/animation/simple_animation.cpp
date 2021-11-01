#include "mcv_platform.h"
#include "entity/entity.h"
#include "components/common/comp_transform.h"
#include "components/common/comp_fsm.h"
#include "skeleton/comp_skeleton.h"
#include "skeleton/game_core_skeleton.h"
#include "animation/animation_callback.h"
#include "fsm/fsm_context.h"
#include "animation/simple_animation.h"
#include "fsm/states/logic/state_logic.h"

namespace fsm
{
    void TSimpleAnimation::load(const json& params)
    {
        name = params.value("anim", name);
        loop = params.value("loop", loop);
        blend_out = params.value("blend_out", blend_out);
        extract_root_motion = params.value("root_motion", extract_root_motion);
        extract_root_yaw = params.value("root_yaw", extract_root_yaw);
        keep_action = params.value("keep_action", keep_action);
        back_cycle = params.value("back_cycle", back_cycle);
        custom_start = params.value("custom_start", custom_start);
        link_variable = params.value("link_variable", link_variable);

        if (custom_start)
        {
            start_time = params.value("start_time", start_time);
        }

        if (params.count("async")) {
            loadAsyncAnim(params);
        }

        if (params.count("callbacks")) {
            addCallbacks(params);
        }

        isValid = true;
    }

    void TSimpleAnimation::loadAsyncAnim(const json& params)
    {
        const json& jAsync = params["async"];
        if (!jAsync.count("name") || !jAsync.count("interval")) return;

        async_anim.isValid = true;
        assert(jAsync["name"].is_array());

        for (int i = 0; i < jAsync["name"].size(); ++i)
            async_anim.names.push_back(jAsync["name"][i]);

        async_anim.interval = jAsync["interval"];
        async_anim.blendIn = jAsync.value("blendIn", async_anim.blendIn);
        async_anim.blendOut = jAsync.value("blendOut", async_anim.blendOut);
        async_anim.randomInterval = jAsync.value("randomInterval", async_anim.randomInterval);

        // In blend anims, only do async when blend factors is equal to this value
        async_anim.restrictToBlendFactor = jAsync.value("restrictToBlendFactor", async_anim.restrictToBlendFactor);
        async_anim.blendFactorRestriction = loadVEC2(jAsync, "blendFactorRestriction");
    }

    void TSimpleAnimation::addCallbacks(const json& params) const
    {
        CGameCoreSkeleton* core = (CGameCoreSkeleton*)TCompFSM::getCurrentCoreSkeleton();
        assert(core);
        if (!core)
            return;

        const json& jCallbacks = params["callbacks"];
        assert(jCallbacks.is_array());
        for (int i = 0; i < jCallbacks.size(); ++i)
        {
            const json& cbNameitem = jCallbacks[i];
            assert(cbNameitem.is_string());
            CCallbackFactory cb_factory = CallbackRegistry.get(cbNameitem);
            CalAnimationCallback* cb = cb_factory();
            auto anim = core->getCoreAnimation(core->getCoreAnimationId(name));
            anim->registerCallback(cb, Time.delta_unscaled);
        }
    }

    void TSimpleAnimation::play(CContext& ctx, const ITransition* transition) const
    {
        CEntity* eOwner = ctx.getOwnerEntity();
        if (!eOwner || !isValid) return;

        TCompSkeleton* c_skel = eOwner->get<TCompSkeleton>();
        CGameCoreSkeleton* core = (CGameCoreSkeleton*)c_skel->model->getCoreModel();
        CalMixer* mixer = c_skel->model->getMixer();

        int anim_id = core->getCoreAnimationId(name);

        /* 
        * Use artist-defined blending times, if not defined, use 
        * transition blendIn and state blendOut
        */

        auto params = core->getAnimationUserParams(anim_id);
        float blendIn = params.blendIn;
        
        if (transition && blendIn == -1.f)
            blendIn = transition->blend_time;

        float blendOut = params.blendOut;

        if (blendOut == -1.f)
            blendOut = blend_out;

        if (!loop) {

            float startTime = custom_start ? start_time : ctx.getTimeInState();
            mixer->executeAction(anim_id, blendIn, blendOut, 1.0, keep_action, extract_root_motion, 
                extract_root_yaw, startTime);

            if (back_cycle.length() > 0) {

                // clear previous cycles
                for (int i = 0; i < c_skel->model->getCoreModel()->getCoreAnimationCount(); ++i) {
                    mixer->clearCycle(i, blendIn);
                }

                int back_cycle_id = core->getCoreAnimationId(back_cycle);
                mixer->blendCycle(back_cycle_id, 1.0f, blendIn);
            }
        }
        else {

            // clear previous cycles
            for (int i = 0; i < c_skel->model->getCoreModel()->getCoreAnimationCount(); ++i) {
                mixer->clearCycle(i, blendIn);
            }

            // Blend new current cycle
            mixer->blendCycle(anim_id, 1.0f, blendIn);
        }
    }

    void TSimpleAnimation::update(CContext& ctx, float dt) const
    {
        CEntity* eOwner = ctx.getOwnerEntity();
        if (!eOwner) return;

        if(async_anim.isValid)
            updateAsync(ctx, dt);

        // Loops don't finish
        if (loop) 
            return;

        TCompSkeleton* c_skel = eOwner->get<TCompSkeleton>();
        auto core = (CGameCoreSkeleton*)c_skel->model->getCoreModel();
        int anim_id = core->getCoreAnimationId(name);
        auto animation = core->getCoreAnimation(anim_id);

        if (ctx.getTimeInState() >= animation->getDuration()) {
            ctx.setStateFinished(true);

            if (link_variable.size()) {
                ctx.setVariableValue(link_variable, false);
            }
        }
    }

    void TSimpleAnimation::stop(CContext& ctx) const
    {
        CEntity* eOwner = ctx.getOwnerEntity();
        if (!eOwner || !isValid) return;

        TCompSkeleton* c_skel = eOwner->get<TCompSkeleton>();
        CGameCoreSkeleton* core = (CGameCoreSkeleton*)c_skel->model->getCoreModel();
        CalMixer* mixer = c_skel->model->getMixer();
        int anim_id = core->getCoreAnimationId(name);

        auto params = core->getAnimationUserParams(anim_id);
        float blendOut = params.blendOut;

        if (blendOut == -1.f)
            blendOut = blend_out;

        mixer->removeAction(anim_id, blendOut);

        if (async_anim.isValid)
            stopAsync(ctx);

        // Stop any linked variable
        if (link_variable.size()) {
            ctx.setVariableValue(link_variable, false);
        }
    }

    void TSimpleAnimation::pause(CContext& ctx) const
    {
        CEntity* eOwner = ctx.getOwnerEntity();
        if (!eOwner || !isValid) return;

        TCompSkeleton* c_skel = eOwner->get<TCompSkeleton>();
        CalMixer* mixer = c_skel->model->getMixer();

        auto anim = mixer->getActionAnim(name);
        if(anim)
            anim->setTimeFactor(0.f);
    }

    void TSimpleAnimation::resume(CContext& ctx) const
    {
        CEntity* eOwner = ctx.getOwnerEntity();
        if (!eOwner || !isValid) return;

        TCompSkeleton* c_skel = eOwner->get<TCompSkeleton>();
        CalMixer* mixer = c_skel->model->getMixer();

        auto anim = mixer->getActionAnim(name);
        if (anim)
            anim->setTimeFactor(1.f);
    }

    void TSimpleAnimation::updateAsync(CContext& ctx, float dt) const
    {
        CEntity* eOwner = ctx.getOwnerEntity();
        if (!eOwner) return;

        int current_id = std::get<int>(ctx.getVariableValue("current_async_id"));

        TCompSkeleton* c_skel = eOwner->get<TCompSkeleton>();
        auto core = (CGameCoreSkeleton*)c_skel->model->getCoreModel();

        if (current_id != -1)
        {
            float timer = std::get<float>(ctx.getVariableValue("loco_async_timer"));
            float interval = std::get<float>(ctx.getVariableValue("loco_async_interval"));

            timer += dt;

            if (timer > interval) {
                timer = 0;
                ctx.setVariableValue("current_async_id", -1);
            }

            ctx.setVariableValue("loco_async_timer", timer);
        }
        else
        {
            // Play animation with random index
            int index = static_cast<int>(Random::unit() * async_anim.names.size());
            c_skel->playAnimation(async_anim.names[index], ANIMTYPE::ACTION, async_anim.blendIn, async_anim.blendOut);

            // Set animation interval
            int async_anim_id = core->getCoreAnimationId(async_anim.names[index]);
            auto animation = core->getCoreAnimation(async_anim_id);
            float async_duration = animation->getDuration();
            float interval = async_anim.getInterval(async_duration);

            ctx.setVariableValue("current_async_id", async_anim_id);
            ctx.setVariableValue("loco_async_interval", interval);
        }
    }

    void TSimpleAnimation::stopAsync(CContext& ctx) const
    {
        CEntity* eOwner = ctx.getOwnerEntity();
        if (!eOwner) return;

        TCompSkeleton* c_skel = eOwner->get<TCompSkeleton>();
        auto core = (CGameCoreSkeleton*)c_skel->model->getCoreModel();
        int current_id = std::get<int>(ctx.getVariableValue("current_async_id"));
        
        if (current_id != -1)
        {
            auto animation = core->getCoreAnimation(current_id);
            c_skel->stopAnimation(animation->getName(), ANIMTYPE::ACTION, async_anim.blendOut);
        }
        
        ctx.setVariableValue("loco_async_timer", 0.f);
    }

    CalAnimation::State TSimpleAnimation::getState(CContext& ctx) const
    {
        CEntity* eOwner = ctx.getOwnerEntity();
        if (!eOwner || !isValid) return CalAnimation::STATE_NONE;

        TCompSkeleton* c_skel = eOwner->get<TCompSkeleton>();
        CalMixer* mixer = c_skel->model->getMixer();
        auto animation = mixer->getActionAnim(name);
        if(!animation) return CalAnimation::STATE_NONE;

        return animation->getState();
    }

    float TSimpleAnimation::getAnimationTime(const CContext& ctx) const
    {
        CEntity* eOwner = ctx.getOwnerEntity();
        if (!eOwner) return 0.f;

        TCompSkeleton* c_skel = eOwner->get<TCompSkeleton>();
        auto core = (CGameCoreSkeleton*)c_skel->model->getCoreModel();
        int id = core->getCoreAnimationId(name);
        auto animation = core->getCoreAnimation(id);

        return animation->getDuration();
    }

    /*  
        Alex: if it's possible to abort the state manually and not only with the 
        stun, it's better to use
            bool stunned = std::get<bool>(ctx.getVariableValue("is_stunned"));
        to check for stun interruptions
    */
    bool TSimpleAnimation::aborted(CContext& ctx) const
    {
        return (getState(ctx) != CalAnimation::STATE_NONE);
    }

    void TSimpleAnimation::renderInMenu(CContext& ctx) const
    {
        ImGui::Text("Name: %s", name.c_str());
    }
}
