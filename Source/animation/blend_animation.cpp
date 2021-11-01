#include "mcv_platform.h"
#include "entity/entity.h"
#include "components/common/comp_transform.h"
#include "skeleton/comp_skeleton.h"
#include "skeleton/game_core_skeleton.h"
#include "animation/animation_callback.h"
#include "fsm/fsm_context.h"
#include "animation/blend_animation.h"
#include "fsm/states/logic/state_logic.h"
#include "components/cameras/comp_camera_shooter.h"

namespace fsm
{
    void TBlendAnimation::load(const json& params)
    {
        name = "blend_animation";
        loop = true; // support blend anims for LOOP animations
        isBlendAnim = true;
        extract_root_motion = params.value("root_motion", extract_root_motion);
        extract_root_yaw = params.value("root_yaw", extract_root_yaw);
        blend_time = params.value("blend_time", blend_time);
        blend_out = params.value("blend_out", blend_out);

        if (params.count("async")) {
            loadAsyncAnim(params);
        }

        const json& jBlendSamples = params["anim_samples"];
        assert(jBlendSamples.is_array());
        interpolator = new CNNInterpolator();

        for (unsigned int i = 0; i < jBlendSamples.size(); ++i) {
            const json& jSample = jBlendSamples[i];

            std::string name = jSample.value("name", "");
            if (!name.length()) fatal("Blend sample has no animation");

            VEC2 blend_point = VEC2::Zero;

            if (!jSample.count("blend_point"))
                fatal("Sample [%s] has no blend point", name.c_str());
            else
            {
                blend_point = loadVEC2(jSample, "blend_point");
            }

            // Update maximum value to normalize later the control points
            float blendFactor = blend_point.Length();
            if (blendFactor > maxBlendPointFactor)
                maxBlendPointFactor = blendFactor;

            SBlendSample bs = { i, name, blend_point };
            blend_samples.push_back(bs);
        }

        // Add control points for the NNI interpolator
        for (auto& bs : blend_samples)
        {
            interpolator->addPoint(bs.id, bs.blendPoint/maxBlendPointFactor);
        }

        isValid = true;
    }

    void TBlendAnimation::play(CContext& ctx, const ITransition* transition) const
    {
        CEntity* eOwner = ctx.getOwnerEntity();
        if (!isValid || !eOwner) return;

        // Remove previous cycles if last state was a loop
        // (either simple or blendanim)

        TCompSkeleton* c_skel = eOwner->get<TCompSkeleton>();
        CalModel* model = c_skel->model;
        CalMixer* mixer = model->getMixer();
        CGameCoreSkeleton* core = (CGameCoreSkeleton*)model->getCoreModel();

        float transitionBlendTime = transition ? transition->blend_time : 0.f;

        // Clean ALL animations
        for (int i = 0; i < c_skel->model->getCoreModel()->getCoreAnimationCount(); ++i) {

            auto params = core->getAnimationUserParams(i);
            float blendOut = params.blendOut;

            if (blendOut == -1.f)
                blendOut = transitionBlendTime;

            mixer->clearCycle(i, blendOut);
        }

        // All weights to Idle
        auto weights = interpolator->getWeights(VEC2::Zero);

        for (auto& w : weights)
        {
            int anim_id = core->getCoreAnimationId(blend_samples[w.id].name);
            auto params = core->getAnimationUserParams(anim_id);
            float blendIn = params.blendIn;

            if (blendIn == -1.f)
                blendIn = transitionBlendTime;

            mixer->blendCycle(anim_id, w.weight, blendIn);
        }
    }

    void TBlendAnimation::update(CContext& ctx, float dt) const
    {
        PROFILE_FUNCTION("Blend animation update");

        CEntity* eOwner = ctx.getOwnerEntity();
        if (!eOwner) return;

        TCompSkeleton* c_skel = eOwner->get<TCompSkeleton>();
        CalMixer* mixer = c_skel->model->getMixer();
        CGameCoreSkeleton* core = (CGameCoreSkeleton*)c_skel->model->getCoreModel();

        // Manage asynchronous animations
        VEC2 blend_factor = getBlendFactor(ctx);
        if (checkAsyncRestrictions(ctx, blend_factor))
            updateAsync(ctx, dt);

        // Custom non-state animations can lock the blendanimation state
        if (c_skel->bsLocked())
            return;
        
        // Get current animation blendtree weights
        auto weights = interpolator->getWeights(blend_factor);
        float sumWeights = 0.f;
        for (auto& w : weights) {
            w.weight = pow(w.weight, 2.f);
            sumWeights += w.weight;
        }

        for (int i = 0; i < blend_samples.size(); ++i) {

            const std::string& name = blend_samples[i].name;
            auto cycle = mixer->getCycleAnim(name);
            int anim_id = core->getCoreAnimationId(name);
            float weight = weights[i].weight / sumWeights;

            // Get blending times --------------------------------
            auto params = core->getAnimationUserParams(anim_id);
            float blendIn = params.blendIn;

            if (blendIn == -1.f)
                blendIn = blend_time;

            float blendOut = params.blendOut;

            if (blendOut == -1.f)
                blendOut = blend_time;
            // ----------------------------------------------------

            if (!mixer->isCycleActive(anim_id) && weight == 0.f)
                continue;

            // Cycle has to be removed
            if (weight <= 0.001f && cycle)
            {
                mixer->clearCycle(anim_id, blendOut);
            }
            else
            {
                // Add new cycle
                if (!cycle)
                {
                    if (!weight)
                        continue;

                    mixer->blendCycle(anim_id, weight, blendIn);
                }
                else
                {
                    // Override cycle only when weights change
                    if (fabsf(cycle->getTargetWeight() - weight) > 0.001f)
                    {
                        // mixer->clearCycle(anim_id, blendOut);
                        mixer->blendCycle(anim_id, weight, blendIn);
                    }
                }
            }
        }
    }

    VEC2 TBlendAnimation::getBlendFactor(CContext& ctx) const
    {
        assert(maxBlendPointFactor != 0.f);
        if (maxBlendPointFactor == 0.f) {
            return VEC2::Zero;
        }

        VEC2 velocity = std::get<VEC2>(ctx.getVariableValue("blend_factor"));
        velocity /= maxBlendPointFactor;

        float velocity_length = velocity.Length();
        velocity.Normalize();

        VEC2 normLength = velocity * clampf(velocity_length, -1, 1);
        return normLength;
    }

    bool TBlendAnimation::checkAsyncRestrictions(CContext& ctx, VEC2 factor) const
    {
        if (!async_anim.isValid)
            return false;

        bool is_ok = true;

        // Discard ALWAYS if playing in Shooter cam
        CEntity* e_camera = getEntityByName("camera_shooter");
        if (e_camera) {
            TCompCameraShooter* shooter_camera = e_camera->get<TCompCameraShooter>();
            is_ok &= !shooter_camera->enabled;
        }

        if (async_anim.restrictToBlendFactor)
            is_ok &= VEC2::Distance(async_anim.blendFactorRestriction, factor) <= 0.001f;

        if (!is_ok) {
            stopAsync(ctx);
        }

        return is_ok;
    }

    void TBlendAnimation::stop(CContext& ctx) const
    {
        if (async_anim.isValid)
            stopAsync(ctx);
    }

    const std::string& TBlendAnimation::getMaxWeightAnimation(CContext& ctx) const
    {
        VEC2 blend_factor = getBlendFactor(ctx);
        auto weights = interpolator->getWeights(blend_factor);

        if (!weights.size() || !blend_samples.size())
            return string_no_animation;

        float max = -100000.f;
        int anim_id = -1;

        for (auto& w : weights) {
            if (fabsf(w.weight) > max) {
                max = w.weight;
                anim_id = w.id;
            }
        }

        if (max < 0.83f || anim_id < 0 || anim_id >= blend_samples.size())
            return string_no_animation;

        return blend_samples[anim_id].name;
    }

    void TBlendAnimation::renderInMenu(CContext& ctx) const
    {
        CEntity* eOwner = ctx.getOwnerEntity();
        if (!eOwner) return;
        TCompSkeleton* c_skel = eOwner->get<TCompSkeleton>();
        CalModel* model = c_skel->model;
        CalMixer* mixer = model->getMixer();
        CGameCoreSkeleton* core = (CGameCoreSkeleton*)model->getCoreModel();
        
        // Use later X, Y for lateral movement
        VEC2 factor = std::get<VEC2>(ctx.getVariableValue("blend_factor"));
        ImGui::Text("Factor: %f,%f", factor.x, factor.y);
        ImGui::Separator();

        auto weights = interpolator->getWeights();

        for (int i = 0; i < blend_samples.size(); ++i) {

            auto anim = mixer->getCycleAnim(blend_samples[i].name);
            ImGui::Text("%s: %f", blend_samples[i].name.c_str(), weights[i].weight);
        }
        ImGui::Separator();
    }
}
