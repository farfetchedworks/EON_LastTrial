#include "mcv_platform.h"
#include "fsm/fsm_context.h"
#include "fsm/states/state_shake.h"
#include "components/messages.h"

namespace fsm
{
    void CStateShake::onLoad(const json& params)
    {
        amount = params.value("amount", amount);
    }

    void CStateShake::onEnter(CContext& ctx, const ITransition* transition) const
    {
        TMsgShake msg;
        msg.amount = amount;
        msg.enabled = true;
        ctx.getOwnerEntity().sendMsg(msg);
    }

    void CStateShake::onExit(CContext& ctx) const
    {
        TMsgShake msg;
        msg.enabled = false;
        ctx.getOwnerEntity().sendMsg(msg);
    }
}
