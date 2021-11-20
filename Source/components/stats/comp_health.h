#pragma once

#include "entity/entity.h"
#include "components/common/comp_base.h"
#include "components/messages.h"

enum class EDamageType {
    SCENARIO,
    COMBAT
};

class TCompHealth : public TCompBase {

    DECL_SIBLING_ACCESS();

    int health                  = 100;
    int lerp_health             = 100;
    int curr_max_health         = 100;
    int max_health              = 200;

    int     lastDamageTaken       = 0;

    float   damageDebugTime       = 1.2f;
    float   elapsedHitTime        = .0f;
    float   debugTimer            = damageDebugTime;

    float   timeSinceLastHit      = .05f; 
    float   lerpHealthWaitTime    = 1.75f;

    bool    is_boss                 = false;
    bool    render_active           = true;
    bool    death_animation_completed = false;
    bool    is_player               = false;

    const interpolators::IInterpolator* interpolator = nullptr;

    void setMaxHealth(int value) { max_health = value; }

public:

    void debugInMenu();
    void renderDebug();
    void load(const json& j, TEntityParseContext& ctx);
    void update(float dt);
    void onEntityCreated();
    
    void onReduceHealth(const TMsgReduceHealth& msg);

    const int getHealth() const { return health; }
    const int getCurrMaxHealth() const { return curr_max_health; }
    const int getMaxHealth() const { return max_health; }
    const float getHealthPercentage() const { return (float)health / (float)max_health; }

    void setRenderActive(bool active, const std::string& boss_name = std::string());
    void setCurrMaxHealth(int value) { curr_max_health = value; }
    void setHealth(int value);

    // returns if the animation has been completed and sets it to completed
    bool checkDeathAnimationCompleted();
    void fillHealth();
    void increaseHealth(int health_points);
    void reduceHealth(int health_points);
    void lerpHealth(float dt);
    bool isDead();

    // Register messages to be called by other entities
    static void registerMsgs() {
        DECL_MSG(TCompHealth, TMsgReduceHealth, onReduceHealth);
    }
};
