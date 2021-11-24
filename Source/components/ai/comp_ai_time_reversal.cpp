#include "mcv_platform.h"
#include "engine.h"
#include "entity/entity_parser.h"
#include "comp_ai_time_reversal.h"
#include "render/draw_primitives.h"
#include "modules/module_physics.h"
#include "audio/module_audio.h"
#include "components/common/comp_transform.h"
#include "components/stats/comp_health.h"
#include "components/common/comp_hierarchy.h"
#include "components/gameplay/comp_game_manager.h"
#include "components/gameplay/comp_lifetime.h"
#include "components/controllers/comp_player_controller.h"
#include "components/render/comp_dissolve.h"

DECL_OBJ_MANAGER("ai_time_reversal", TCompAITimeReversal)

TCompAITimeReversal::~TCompAITimeReversal() {
    delete[] circular_buffer;
}

void TCompAITimeReversal::load(const json& j, TEntityParseContext& ctx)
{
    max_seconds = j.value("max_seconds", max_seconds);
    rewind_speed = j.value("rewind_speed", rewind_speed);
    holo_entity_name = j.value("holo_entity_name", holo_entity_name);
    holo_prefab = j.value("holo_prefab", holo_prefab);
    if (holo_prefab != "") spawn_holo = true;
}

void TCompAITimeReversal::onEntityCreated()
{
    h_transform = get<TCompTransform>();
    h_collider = get<TCompCollider>();
    h_enemy_health = get<TCompHealth>();
    buffer_size = max_seconds * SHOTS_PER_SECOND;
    circular_buffer = new STRAIShot[buffer_size];
}

void TCompAITimeReversal::update(float dt)
{
    PROFILE_FUNCTION("AI Time Reversal Update");

    dt = Time.delta_unscaled;

    // When not rewinding we add new shot every TIME_PER_SHOT seconds
    if (!is_rewinding) {
        
        time_reversal_timer += dt;

        if (time_reversal_timer >= TIME_PER_SHOT) {
            generateShot();
        }
    }
    else {

        // cache
        if (!h_game_manager.isValid())
            h_game_manager = GameManager->get<TCompGameManager>();

        TCompGameManager* c_game_manager = h_game_manager;
        float time_scale_factor = c_game_manager->getTimeScaleFactor();

        time_reversal_timer += rewind_speed * dt;

        //// Used to calculate total elapsed time for final damage and impulse
        //force_scale_timer += rewind_speed * dt;

        // We consume shot every TIME_PER_SHOT seconds
        if (time_reversal_timer >= TIME_PER_SHOT) {
            consumeShot();

            // We want to keep time_reversal_timer in range [0, TIME_PER_SHOT] so we can use it later for interpolation
            time_reversal_timer -= TIME_PER_SHOT;

            // We interpolate between pairs of shots, so number of interpolations is (generated_shots - 1)
            if (generated_shots <= 1) {
                stopRewinding();
            }
        }

        TCompCollider* c_collider = h_collider;
        TCompTransform* transform = h_transform;

        // The division gives us a value in range [0, 1]
        const STRAIShot lerpValue = interpolatedValue(time_reversal_timer / TIME_PER_SHOT);
        c_collider->setFootPosition(lerpValue.position);
        transform->setRotation(QUAT::CreateFromAxisAngle(VEC3(0, 1, 0), lerpValue.yaw));

    }
}

bool TCompAITimeReversal::startRewinding()
{
    if (is_rewinding || generated_shots != buffer_size) {
        return false;
    }

    TCompHealth* c_health = h_enemy_health;

    // Spawn hologram
    if (spawn_holo) {
      CHandle h_Holo = getEntityByName(holo_entity_name);

      if (!h_Holo.isValid()) {
          spawnHolo();
      }
    }

    stop_timer = -1.f;
    is_rewinding = true;
    rewind_index = current_index == 0 ? (buffer_size - 1) : current_index - 1;

    // In case player's health is reduced between last shot added and time reversal start
    c_health->setHealth(circular_buffer[rewind_index].current_health);

    // First interpolate between player current position and first shot
    float project_lerp_value = time_reversal_timer / TIME_PER_SHOT;
    TCompTransform* comp_transform = h_transform;
    VEC3 project_pawn_pos = projectLerp(circular_buffer[rewind_index].position, comp_transform->getPosition(), project_lerp_value);
    lerp_origin = { project_pawn_pos, vectorToYaw(comp_transform->getForward()), c_health->getHealth() };
    lerp_target = circular_buffer[rewind_index];

    TCompCollider* comp_collider = h_collider;

    TCompDissolveEffect* c_dissolve = get<TCompDissolveEffect>();
    if (c_dissolve) {
        c_dissolve->setRemoveColliders(false);
        c_dissolve->enable(1.4f, 2.f, true);
    }

    // Needed to avoid collision with environment
    EnginePhysics.setupFilteringOnAllShapesOfActor(comp_collider->actor, CModulePhysics::FilterGroup::Enemy, CModulePhysics::FilterGroup::Scenario);

    // In case we are in the middle of a dash or getting hit, reset physics state
    comp_collider->active_force = false;
    EnginePhysics.setSimulationDisabled(comp_collider->actor, false);
    EnginePhysics.setSimulationDisabled(comp_collider->force_actor, true);
    EnginePhysics.setupFilteringOnAllShapesOfActor(comp_collider->force_actor, CModulePhysics::FilterGroup::None, CModulePhysics::FilterGroup::None);

    return true;
}

void TCompAITimeReversal::stopRewinding()
{
    // Restore default filters
    TCompCollider* c_collider = h_collider;
    EnginePhysics.setupFilteringOnAllShapesOfActor(c_collider->actor, CModulePhysics::FilterGroup::Enemy, CModulePhysics::FilterGroup::All);

    // Reset internal variables
    is_rewinding = false;
    generated_shots = 0;
    stop_timer = 0.f;
    
    // Begin lifetime for Eon copy if created
    if (spawn_holo) {
      CEntity* e_holo = getEntityByName(holo_entity_name);

      if (e_holo) {
          TCompLifetime* lTime = e_holo->get<TCompLifetime>();
          lTime->init();
      }
    }
}

STRAIShot TCompAITimeReversal::interpolatedValue(float lerp_value)
{
    VEC3 position_lerp = lerp(lerp_origin.position, lerp_target.position, lerp_value);
    float yaw_lerp = lerpRadians(lerp_origin.yaw, lerp_target.yaw, 1.0f, lerp_value);

    return { position_lerp, yaw_lerp };
}

void TCompAITimeReversal::generateShot()
{
    time_reversal_timer -= TIME_PER_SHOT;
    TCompTransform* transform = h_transform;
    TCompHealth* c_health = h_enemy_health;
    TCompCollider* c_collider = h_collider;

    STRAIShot shot = { transform->getPosition(), vectorToYaw(transform->getForward()), c_health->getHealth() };

    addShot(shot);
}

void TCompAITimeReversal::addShot(const STRAIShot& shot)
{
    circular_buffer[current_index] = shot;
    current_index = (current_index + 1) % buffer_size;

    if (generated_shots < buffer_size) {
        generated_shots++;
    }
}

void TCompAITimeReversal::consumeShot()
{
    lerp_origin = circular_buffer[rewind_index];
    rewind_index = rewind_index == 0 ? (buffer_size - 1) : rewind_index - 1;
    lerp_target = circular_buffer[rewind_index];

    TCompHealth* c_health = h_enemy_health;
    c_health->setHealth(lerp_target.current_health);

    generated_shots--;
}

void TCompAITimeReversal::spawnHolo()
{
    TCompTransform* transform = h_transform;       
    spawn(holo_prefab, *transform);

    // Set lock-on target to the holo
    CEntity* e_hologram = getEntityByName(holo_entity_name);

    TCompTransform* holo_trans = e_hologram->get<TCompTransform>();
    holo_trans->setPosition(transform->getPosition());
    holo_trans->setRotation(transform->getRotation());

    CEntity* e_player = getEntityByName("player");
    TCompPlayerController* c_player_cont = e_player->get<TCompPlayerController>();
    if (c_player_cont->is_locked_on)
      c_player_cont->lockOnToTarget(e_hologram);
}

void TCompAITimeReversal::debugInMenu()
{
    ImGui::Text(is_rewinding ? "REWINDING" : "NOT REWINDING");
    ImGui::ProgressBar(generated_shots / (float)buffer_size, ImVec2(-1, 0));
}

void TCompAITimeReversal::renderDebug()
{

}