#include "mcv_platform.h"
#include "fsm/states/anim/state_blend_animation.h"
#include "fsm/fsm_context.h"
#include "entity/entity.h"
#include "components/common/comp_transform.h"
#include "skeleton/comp_skeleton.h"
#include "skeleton/game_core_skeleton.h"

namespace fsm
{
    void CStateBlendAnimation::onLoad(const json& params)
    {
        anim.load(params);
    }

    void CStateBlendAnimation::onEnter(CContext& ctx, const ITransition* transition) const
    {
        anim.play(ctx, transition);
    }

    void CStateBlendAnimation::onUpdate(CContext& ctx, float dt) const
    {
        anim.update(ctx, dt);
    }

    void CStateBlendAnimation::renderInMenu(CContext& ctx, const std::string& prefix) const
    {
        CStateAnimation::renderInMenu(ctx, prefix);

        if (anim.isValid)
            anim.renderInMenu(ctx);
    }
}
