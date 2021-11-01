#pragma once

#include "entity/entity.h"
#include "components/common/comp_base.h"
#include "components/messages.h"

class TCompMessageArea : public TCompBase {

    DECL_SIBLING_ACCESS();

private:
  bool one_time_activation = true;          // If set to true, it can be activated only once, then it will disappear
  bool has_been_activated = false;          // Set to true the first time it was activated
  bool is_active = false;
  bool pause_game = false;                  // if set to true, the game will be paused until the player interacts with the message
  
  std::string message_id = "";            // Message to display

  float current_time = 0.f;
  float duration = 2.0;                     // Time during which the message will appear. If duration is 0, the message is permanent and disappears when the player exits the trigger

  void showMessage();
  void clearMessage();

public:

  void load(const json& j, TEntityParseContext& ctx);
  void onEntityCreated();
  void update(float dt);
  void debugInMenu();

  void onActivateMsg(const TMsgActivateMsgArea& msg);
  void onDeactivateMsg(const TMsgDeactivateMsgArea& msg);

  // Register messages to be called by other entities
  static void registerMsgs() {
    DECL_MSG(TCompMessageArea, TMsgActivateMsgArea, onActivateMsg);
    DECL_MSG(TCompMessageArea, TMsgDeactivateMsgArea, onDeactivateMsg);
  }
};
