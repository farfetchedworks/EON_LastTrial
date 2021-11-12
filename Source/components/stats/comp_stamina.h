#pragma once

#include "components/common/comp_base.h"
#include "components/controllers/comp_player_controller.h"
#include "components/controllers/pawn_actions.h"

class TCompStamina: public TCompBase {

    DECL_SIBLING_ACCESS();

    float max_stamina = 100.f;
    float curr_max_stamina = 100.f;
    float current_stamina = curr_max_stamina;
    float recovery_points = 2.f;
    float recovery_frequency = 0.03f;
    float recovery_penalization = 0.1f;
    float recovery_penalization_at_0 = 0.25f;
    float time_elapsed = 0.f;

    bool recovery_penalized = false;
    bool last_can_recover = false;

    struct EStaminaData {
        int stamina_cost = 0;
        std::string action_name = {};
    };
    
    // Render in table (ImGui)
    std::map<EAction, EStaminaData> stamina_costs;

public:

  void load(const json& j, TEntityParseContext& ctx);
  void update(float dt);
  void debugInMenu();
  void renderDebug();

  float getCurrentStamina();

  void fillStamina();
  void reduceStamina(EAction);

  // Called by the player controller when an action has finished to start recovering stamina
  void recoverStamina(bool can_recover, float dt);

  // Returns true if the player has the max stamina
  bool hasMaxStamina();
  void setMaxStamina(int max) { max_stamina = (float)max; };
  void setCurrMaxStamina(int max) { curr_max_stamina = (float)max; };

  // Returns true if the player has enough stamina to perform the action
  bool hasStamina(EAction);

  // Returns true if the player has stamina
  bool hasStamina();
};
