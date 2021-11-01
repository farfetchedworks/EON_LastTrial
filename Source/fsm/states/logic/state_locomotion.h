#pragma once
#include "entity/entity.h"
#include "fsm/fsm_context.h"
#include "fsm/fsm_parser.h"
#include "fsm/states/logic/state_logic.h"
#include "animation/blend_animation.h"

using namespace fsm;

class CStateLocomotion : public CStateBaseLogic
{
protected:
    // Each state can have a simple or blend animation
    TBlendAnimation anim;

public:
    void onLoad(const json& params);
    void onEnter(CContext& ctx, const ITransition* transition) const;
    void onExit(CContext& ctx) const;
    void onUpdate(CContext& ctx, float dt) const;
    void renderInMenu(CContext& ctx, const std::string& prefix = "") const;

    TBlendAnimation& getAnimation() { 
        return anim; 
    }
};