#include "mcv_platform.h"
#include "fsm/states/state_move.h"
#include "fsm/fsm_context.h"
#include "entity/entity.h"
#include "components/common/comp_transform.h"

namespace fsm
{
    void CStateMove::onUpdate(CContext& ctx, float dt) const
    {
        CEntity* eOwner = ctx.getOwnerEntity();
        if (!eOwner) return;

        const float timeLeft = duration - ctx.getTimeInState();
        const float frameDt = std::min(dt, timeLeft);
        const VEC3 frameOffset = duration > 0.f ? offset * (frameDt / duration) : offset;

        TCompTransform* cTransform = eOwner->get<TCompTransform>();
        const VEC3 newPos = cTransform->getPosition() + frameOffset;
        cTransform->setPosition(newPos);

        if (timeLeft <= 0.f)
        {
            ctx.setStateFinished(true);
        }
    }

    void CStateMove::onLoad(const json& params)
    {
        offset = loadVEC3(params, "offset");
        duration = params.value("duration", duration);
    }
}
