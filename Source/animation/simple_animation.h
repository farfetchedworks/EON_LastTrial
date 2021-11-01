#pragma once
#include "Cal3d/animation.h"

class CalCoreAnimation;

namespace fsm
{
    struct TSimpleAnimation
    {
    protected:
        struct TAsyncAnim {
            bool isValid = false;
            bool randomInterval = false;
            float blendIn = 0.2f;
            float blendOut = 0.2f;
            float interval = 0.f;

            std::vector<std::string> names;

            bool restrictToBlendFactor = true;
            VEC2 blendFactorRestriction = VEC2::Zero;
            
            float getInterval(float anim_duration) const {
                return anim_duration + interval + (randomInterval ? Random::unit() * interval : 0.f);
            }
        };

    public:

        virtual void load(const json& params);
        virtual void play(CContext& ctx, const ITransition* transition) const;
        virtual void stop(CContext& ctx) const;
        virtual void update(CContext& ctx, float dt) const;
        virtual void renderInMenu(CContext& ctx) const;

        virtual void pause(CContext& ctx) const;
        virtual void resume(CContext& ctx) const;
        
        void loadAsyncAnim(const json& params);
        void updateAsync(CContext& ctx, float dt) const;
        void stopAsync(CContext& ctx) const;
        
        void addCallbacks(const json& params) const;

        CalAnimation::State getState(CContext& ctx) const;
        float getAnimationTime(const CContext& ctx) const;

        // check if animation finished or aborted at some point
        bool aborted(CContext& ctx) const;

        bool isValid             = false;
        bool loop                = false;
        bool extract_root_motion = false;
        bool extract_root_yaw    = false;
        bool keep_action         = false;
        bool custom_start        = false;
        bool isBlendAnim         = false;

        float blend_out     = 0.f;
        mutable float start_time    = 0.f;

        // Blink actions or any action played random/using a interval
        // over the main animation
        TAsyncAnim async_anim;

        std::string link_variable   = "";
        std::string back_cycle      = "";
        std::string name            = "";
    };
}