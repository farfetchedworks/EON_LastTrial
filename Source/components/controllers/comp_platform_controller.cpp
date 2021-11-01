#include "mcv_platform.h"
#include "components/common/comp_base.h"
#include "components/common/comp_transform.h"
#include "components/messages.h"
#include "entity/entity.h"
#include "engine.h"
#include "input/input_module.h"

struct TCompPlatformController : public TCompBase
{
    DECL_SIBLING_ACCESS();

    static void registerMsgs()
    {
        DECL_MSG(TCompPlatformController, TMsgShake, onShake);
    }

    bool enabled = false;
    VEC3 initialPosition = VEC3::Zero;
    float shakeAmount = 0.f;
    bool shaking = false;
    
    void update(float dt)
    {
        if (CEngine::get().getInput()->getKey(VK_SPACE).getsPressed())
        {
            enabled = !enabled;

            TMsgFSMVariable msg;
            msg.name = "enabled";
            msg.value = enabled;
            CHandle(this).getOwner().sendMsg(msg);
        }

        if (shaking)
        {
            const VEC3 offset(Random::unit() * shakeAmount, 0.f, Random::unit() * shakeAmount);
            TCompTransform* cTransform = get<TCompTransform>();
            cTransform->setPosition(initialPosition + offset);
        }
    }

    void onShake(const TMsgShake& msg)
    {
        TCompTransform* cTransform = get<TCompTransform>();
        if (msg.enabled)
        {
            initialPosition = cTransform->getPosition();
            shakeAmount = msg.amount;
        }
        else
        {
            cTransform->setPosition(initialPosition);
        }

        shaking = msg.enabled;
    }

};

DECL_OBJ_MANAGER("platform_controller", TCompPlatformController)