#pragma once

#include "components/common/comp_base.h"
#include "components/controllers/pawn_actions.h"
#include "entity/entity.h"

namespace fsm {
    class IState;
}

// This makes rewinding independent of fps and also saves
// a lot of memory. When rewinding we interpolate between shots
constexpr int SHOTS_PER_SECOND = 10;
constexpr float TIME_PER_SHOT = 1.0f / static_cast<float>(SHOTS_PER_SECOND);

struct STRShot {
    VEC3 position = {};
    float yaw = 0;
    int current_health = 0;
    const fsm::IState* state = nullptr;
    float time_in_anim = 0.0f;
    VEC3 linear_velocity;
    MAT44 bones[MAX_SUPPORTED_BONES];
};

enum class eEffectState{
    NONE = 0,
    FADE_IN,
    FADE_OUT
};

class TCompTimeReversal : public TCompBase {

    DECL_SIBLING_ACCESS();

private:
    // Cached handlers
    CHandle h_transform;
    CHandle h_collider;
    CHandle h_player;
    CHandle h_player_health;
    CHandle h_skeleton;
    CHandle h_fsm;
    CHandle h_game_manager;
    CHandle h_holo;

    bool is_rewinding               = false;
    bool enabled                    = true;
    eEffectState rendering_effect   = eEffectState::NONE;

    bool invisible_effect = false;
    bool holo_created = false;
    std::string original_material;

    int warp_consumption = 2;
    int max_seconds = 5;
    float rewind_speed = 1.f;

    int buffer_size = 0;
    int current_index = 0; // current circular buffer index
    int rewind_index = 0; // reduced each shot when rewinding util its equal to current_index
    int generated_shots = 0; // used to allow rewinding before max_seconds have elapsed since last rewinding
    STRShot* circular_buffer = nullptr;

    // Used to interpolate between shots
    STRShot lerp_origin;
    STRShot lerp_target;

    float time_reversal_timer = 0.f;
    float force_scale_timer = 0.f;

    // This has to be at -1 so its not used until its 0
    float stop_timer = -1.f;

    int bones_size = 0;

    void generateShot();
    void addShot(const STRShot& shot);
    void consumeShot();

    void spawnHolo();

    // Interpolates between two shots, lerp_factor must go from 0 to 1
    STRShot interpolatedValue(float lerp_value);

public:

    ~TCompTimeReversal();
    void load(const json& j, TEntityParseContext& ctx);
    void onEntityCreated();
    void update(float dt);
    void debugInMenu();
    void renderDebug();

    void disable();
    bool startRewinding();
    void stopRewinding();
    bool isRewinding() { return is_rewinding; }
    int getWarpConsumption() { return warp_consumption; }
    void clearBuffer() { generated_shots = 0; }
    void renderEffect(bool state);
    void updateObjectCte(CShaderCte<CtesObject>& cte);
    void removeInvisibleEffect();
};
