#include "mcv_platform.h"
#include "entity/entity.h"
#include "components/controllers/comp_ai_controller_base.h"

class TCompAIControllerRanged : public TCompAIControllerBase {

public:
    enum EAction
    {
        IDLE,
        MOVE,
        RECEIVE_DMG,
        RANGE_ATTACK,
        DASH,
        DASH_STRIKE,
        HEAL,
        IDLE_WAR,
        NUM_ACTIONS
    };
};
