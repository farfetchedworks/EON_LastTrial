#pragma once

#include "animation/simple_animation.h"
#include "nn_interpolator.h"

namespace fsm
{
    struct TBlendAnimation : public TSimpleAnimation
    {
        std::string string_no_animation = "string_no_animation";

    private:

        struct SBlendSample {
            unsigned int id;
            const std::string name = "";
            VEC2 blendPoint;
        };

        float blend_time = 0.25f;
        float maxBlendPointFactor = 0.f;

        CNNInterpolator* interpolator = nullptr;
        std::vector<SBlendSample> blend_samples;

        bool checkAsyncRestrictions(CContext& ctx, VEC2 factor) const;
        VEC2 getBlendFactor(CContext& ctx) const;

    public:

        ~TBlendAnimation() { 
            if (interpolator) 
                delete interpolator; 
            interpolator = nullptr; 
        }

        void load(const json& params) override;
        void play(CContext& ctx, const ITransition* transition) const override;
        void stop(CContext& ctx) const override;
        void update(CContext& ctx, float dt) const override;
        void renderInMenu(CContext& ctx) const override;

        const std::string& getMaxWeightAnimation(CContext& ctx) const;
    };
}