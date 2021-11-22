#pragma once

#include "components/common/comp_base.h"
#include "components/controllers/pawn_actions.h"
#include "entity/entity.h"

// This makes rewinding independent of fps and also saves
// a lot of memory. When rewinding we interpolate between shots
constexpr int SHOTS_PER_SECOND = 1;
constexpr float TIME_PER_SHOT = 1.0f / static_cast<float>(SHOTS_PER_SECOND);

struct STRAIShot {
    VEC3 position = {};
    float yaw = 0;
    int current_health = 0;
};

class TCompAITimeReversal : public TCompBase {

    DECL_SIBLING_ACCESS();

private:
    // Cached handlers
    CHandle h_transform;
    CHandle h_collider;
    CHandle h_enemy_health;
    CHandle h_game_manager;
    CHandle h_holo;

    bool is_rewinding = false;
    bool spawn_holo = false;

    std::string holo_entity_name = std::string();
    std::string holo_prefab = std::string();

    int max_seconds = 5;
    float rewind_speed = 1.f;

    int buffer_size = 0;
    int current_index = 0; // current circular buffer index
    int rewind_index = 0; // reduced each shot when rewinding util its equal to current_index
    int generated_shots = 0; // used to allow rewinding before max_seconds have elapsed since last rewinding
    STRAIShot* circular_buffer = nullptr;

    // Used to interpolate between shots
    STRAIShot lerp_origin;
    STRAIShot lerp_target;

    float time_reversal_timer = 0.f;

    // This has to be at -1 so its not used until its 0
    float stop_timer = -1.f;

    void generateShot();
    void addShot(const STRAIShot& shot);
    void consumeShot();

    void spawnHolo();

    // Interpolates between two shots, lerp_factor must go from 0 to 1
    STRAIShot interpolatedValue(float lerp_value);

public:

    ~TCompAITimeReversal();
    void load(const json& j, TEntityParseContext& ctx);
    void onEntityCreated();
    void update(float dt);
    void debugInMenu();
    void renderDebug();

    bool startRewinding();
    void stopRewinding();
    bool isRewinding() { return is_rewinding; }
    void clearBuffer() { generated_shots = 0; }
};
