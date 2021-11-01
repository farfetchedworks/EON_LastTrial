#include "mcv_platform.h"
#include "engine.h"
#include "entity/entity.h"
#include "fsm/fsm_context.h"
#include "fsm/fsm_parser.h"
#include "fsm/states/logic/state_logic.h"
#include "modules/module_physics.h"
#include "components/common/comp_transform.h"
#include "components/controllers/comp_player_controller.h"

// ALEX: NOT BEING USED NOW, SINCE IT'S EMPTY, THE STATE IS A
// SIMPLE ANIMATION STATE AND I USE LINK_VARIABLE
// IN THE FSM TO DISABLE THE CONDITION WHEN THE STATE ENDS

using namespace fsm;

class CStateInteract : public CStateBaseLogic
{
public:
  void onLoad(const json& params)
  {
      anim.load(params);
      loadStateProperties(params);
  }

  void onEnter(CContext& ctx, const ITransition* transition) const
  {
    anim.play(ctx, transition);
  }

  void onExit(CContext& ctx) const
  {
    anim.stop(ctx);
  }

  void onUpdate(CContext& ctx, float dt) const
  {
    anim.update(ctx, dt);
  }
};

REGISTER_STATE("interact", CStateInteract)