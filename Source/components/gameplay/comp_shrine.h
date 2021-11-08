#pragma once

#include "entity/entity.h"
#include "components/common/comp_base.h"
#include "components/messages.h"

class TCompShrine : public TCompBase {

    DECL_SIBLING_ACCESS();

private:

    struct PrayPos {
        VEC3 collider;
        VEC3 player;
    };

    PrayPos pray_pos;
    std::string shrine_info = "";
    bool active = false;
    int num_enemies = 0;

    float acceptance_dist = 0.f;

public:

    void load(const json& j, TEntityParseContext& ctx);
    void onEntityCreated();
    void debugInMenu();
    void renderDebug();
    void restorePlayer();
    bool resolve();

    bool isActive() { return active; }
    bool canPray() { return num_enemies == 0; }
    const PrayPos& getPrayPos() { return pray_pos; }

    void onPray(const TMsgShrinePray& msg);
    void onEnemyEnter(const TMsgEnemyEnter& msg);
    void onEnemyExit(const TMsgEnemyExit& msg);

    // Register messages to be called by other entities
    static void registerMsgs() {
        DECL_MSG(TCompShrine, TMsgShrinePray, onPray);
        DECL_MSG(TCompShrine, TMsgEnemyEnter, onEnemyEnter);
        DECL_MSG(TCompShrine, TMsgEnemyExit, onEnemyExit);
    }

};
