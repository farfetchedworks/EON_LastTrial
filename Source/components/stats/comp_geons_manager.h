#pragma once

#include "entity/entity.h"
#include "components/common/comp_base.h"
#include "components/messages.h"

class TCompGeonsManager: public TCompBase {

    DECL_SIBLING_ACCESS();

private:
    int current_geons       = 0;
    int phase               = 1;
    int total_phases        = 1;
    int first_phase_req     = 0;
    float increase_ratio    = 0.f;

    std::map<int, int> phase_requirements;

public:

    static void registerMsgs() {
        DECL_MSG(TCompGeonsManager, TMsgEnemyDied, onEnemyDied);
    }

    void onEnemyDied(const TMsgEnemyDied& msg);
    void renderDebug();
    void debugInMenu();
    void load(const json& j, TEntityParseContext& ctx);

    int getPhase() { return phase; }
    void setPhase(int new_phase);
    void increasePhase();
    void addGeons(int geons);
};
