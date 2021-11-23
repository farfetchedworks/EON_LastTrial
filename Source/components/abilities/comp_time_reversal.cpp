#include "mcv_platform.h"
#include "components/abilities/comp_time_reversal.h"
#include "engine.h"
#include "modules/module_physics.h"
#include "audio/module_audio.h"
#include "components/common/comp_transform.h"
#include "skeleton/comp_skeleton.h"
#include "entity/entity_parser.h"
#include "components/stats/comp_health.h"
#include "components/stats/comp_warp_energy.h"
#include "components/controllers/comp_player_controller.h"
#include "components/common/comp_fsm.h"
#include "components/common/comp_hierarchy.h"
#include "components/gameplay/comp_game_manager.h"
#include "components/gameplay/comp_lifetime.h"
#include "components/cameras/comp_camera_follow.h"
#include "components/render/comp_dissolve.h"
#include "render/draw_primitives.h"
#include "fsm/fsm_state.h"
#include "cal3d/cal3d.h"
#include "ui/ui_module.h"
#include "ui/widgets/ui_image.h"

DECL_OBJ_MANAGER("time_reversal", TCompTimeReversal)

extern CShaderCte<CtesWorld> cte_world;

static const char* FMOD_PARAM_END = "Time_Reversal_End";
static const char* FMOD_PARAM_WARP = "Eon_Inside_Warp";
static const char* FMOD_EVENT = "CHA/Eon/AT/Eon_Time_Reversal";

TCompTimeReversal::~TCompTimeReversal() {
    delete[] circular_buffer;
}

void TCompTimeReversal::load(const json& j, TEntityParseContext& ctx)
{
    warp_consumption = j.value("warp_consumption", warp_consumption);
    max_seconds = j.value("max_seconds", max_seconds);
    rewind_speed = j.value("rewind_speed", rewind_speed);
    enabled = j.value("enabled", enabled);
}

void TCompTimeReversal::onEntityCreated()
{
    h_transform = get<TCompTransform>();
    h_collider = get<TCompCollider>();
    h_player = get<TCompPlayerController>();
    h_player_health = get<TCompHealth>();
    h_skeleton = get<TCompSkeleton>();
    h_fsm = get<TCompFSM>();
    h_game_manager = GameManager->get<TCompGameManager>();

    TCompSkeleton* c_skeleton = h_skeleton;
    bones_size = (int)c_skeleton->model->getSkeleton()->getVectorBone().size();

    buffer_size = max_seconds * SHOTS_PER_SECOND;
    circular_buffer = new STRShot[buffer_size];
}

void TCompTimeReversal::disable()
{
    cte_world.timeReversal_rewinding = 0.f;
    cte_world.timeReversal_rewinding_time = 0.f;

    time_reversal_timer = 0.f;
    force_scale_timer = 0.f;
    rendering_effect = eEffectState::NONE;
    enabled = false;
    clearBuffer();

    // Update UI
    {
        ui::CWidget* w = EngineUI.getWidgetFrom("eon_hud", "time_reversal_bar");
        assert(w);
        ui::CWidget* wChild = w->getChildByName("bar_fill");
        if (wChild) {
            ui::CImage* fill = static_cast<ui::CImage*>(wChild);
            ui::TImageParams& params = fill->imageParams;
            params.alpha_cut = 0.f;
        }
    }
}

void TCompTimeReversal::renderEffect(bool state)
{
    rendering_effect = state ? eEffectState::FADE_IN : eEffectState::FADE_OUT;
    if (state)
    {
        cte_world.timeReversal_rewinding = 1.f;
        EngineAudio.setGlobalRTPC(FMOD_PARAM_END, 0);
        EngineAudio.postEvent(FMOD_EVENT);
    }
}

void TCompTimeReversal::update(float dt)
{
    if (!enabled)
        return;

    PROFILE_FUNCTION("Time Reversal Update");

    dt = Time.delta_unscaled;

    if (rendering_effect != eEffectState::NONE)
    {
        if (rendering_effect == eEffectState::FADE_IN)
        {
            cte_world.timeReversal_rewinding_time = damp(cte_world.timeReversal_rewinding_time, 0.3f, 2.f, dt);
        }
        else if (rendering_effect == eEffectState::FADE_OUT)
        {
            cte_world.timeReversal_rewinding_time = damp(cte_world.timeReversal_rewinding_time, 0.f, 2.f, dt);
            if (cte_world.timeReversal_rewinding_time == 0.f)
            {
                rendering_effect = eEffectState::NONE;
                cte_world.timeReversal_rewinding = 0.f;
                EngineAudio.setGlobalRTPC(FMOD_PARAM_END, 1);
            }
        }

        return;
    }

    // When not rewinding we add new shot every TIME_PER_SHOT seconds
    if (!is_rewinding) {

        // To blend out the effect
        if (stop_timer != -1.f) {
            cte_world.timeReversal_rewinding_time = -stop_timer;
            stop_timer += dt;

            if (stop_timer > 1.f) {
                cte_world.timeReversal_rewinding = 0.f;
                cte_world.timeReversal_rewinding_time = 0.f;
                stop_timer = -1.f;
            }
        }
        
        time_reversal_timer += dt;

        if (time_reversal_timer >= TIME_PER_SHOT) {
            generateShot();
        }
    }
    else {

        TCompGameManager* c_game_manager = h_game_manager;
        float time_scale_factor = c_game_manager->getTimeScaleFactor();

        time_reversal_timer += rewind_speed * dt;

        // Used to calculate total elapsed time for final damage and impulse
        force_scale_timer += rewind_speed * dt;

        // We consume shot every TIME_PER_SHOT seconds
        if (time_reversal_timer >= TIME_PER_SHOT) {
            consumeShot();

            // We want to keep time_reversal_timer in range [0, TIME_PER_SHOT] so we can use it later for interpolation
            time_reversal_timer -= TIME_PER_SHOT;

            // We interpolate between pairs of shots, so number of interpolations is (generated_shots - 1)
            if (generated_shots == 1) {
                stopRewinding();
            }
        }

        TCompCollider* c_collider = h_collider;
        TCompTransform* transform = h_transform;

        // The division gives us a value in range [0, 1]
        const STRShot lerpValue = interpolatedValue(time_reversal_timer / TIME_PER_SHOT);
        c_collider->setFootPosition(lerpValue.position);
        transform->setRotation(QUAT::CreateFromAxisAngle(VEC3(0, 1, 0), lerpValue.yaw));

        cte_world.timeReversal_rewinding_time = force_scale_timer;
    }

    // Update UI
    {
        float pct = (float)generated_shots / (float)buffer_size;
        ui::CWidget* w = EngineUI.getWidgetFrom("eon_hud", "time_reversal_bar");
        assert(w);
        ui::CWidget* wChild = w->getChildByName("bar_fill");
        if (wChild) {
            ui::CImage* fill = static_cast<ui::CImage*>(wChild);
            ui::TImageParams& params = fill->imageParams;
            params.alpha_cut = pct;
        }
    }
}

bool TCompTimeReversal::startRewinding()
{
    if (is_rewinding) {
        dbg("Already rewinding!\n");
        return false;
    }

    cte_world.timeReversal_rewinding = 1.f;

    TCompHealth* c_health = h_player_health;
    CHandle hEonHolo = getEntityByName("EonHolo");

    if (!hEonHolo.isValid()) {
        spawnHolo();
    }

    stop_timer = -1.f;
    is_rewinding = true;
    rewind_index = current_index == 0 ? (buffer_size - 1) : current_index - 1;

    TCompSkeleton* c_skeleton = h_skeleton;
    c_skeleton->custom_bone_update = true;

    // In case player's health is reduced between last shot added and time reversal start
    c_health->setHealth(circular_buffer[rewind_index].current_health);

    // First interpolate between player current position and first shot
    float project_lerp_value = time_reversal_timer / TIME_PER_SHOT;
    TCompTransform* comp_transform = h_transform;
    VEC3 project_pawn_pos = projectLerp(circular_buffer[rewind_index].position, comp_transform->getPosition(), project_lerp_value);
    lerp_origin = { project_pawn_pos, vectorToYaw(comp_transform->getForward()), c_health->getHealth() };

    for (int bone_idx = 0; bone_idx < bones_size; ++bone_idx) {
        lerp_origin.bones[bone_idx] = projectLerp(circular_buffer[rewind_index].bones[bone_idx], c_skeleton->cb_bones.bones[bone_idx], project_lerp_value);
    }

    //std::copy(std::begin(c_skeleton->cb_bones.bones), std::end(c_skeleton->cb_bones.bones), std::begin(lerp_origin.bones));

    lerp_target = circular_buffer[rewind_index];

    TCompCollider* comp_collider = h_collider;

    // Needed to avoid collision with environment
    EnginePhysics.setupFilteringOnAllShapesOfActor(comp_collider->actor, CModulePhysics::FilterGroup::Player, CModulePhysics::FilterGroup::Scenario);

    // In case we are in the middle of a dash or getting hit, reset physics state
    comp_collider->active_force = false;
    EnginePhysics.setSimulationDisabled(comp_collider->actor, false);
    EnginePhysics.setSimulationDisabled(comp_collider->force_actor, true);
    EnginePhysics.setupFilteringOnAllShapesOfActor(comp_collider->force_actor, CModulePhysics::FilterGroup::None, CModulePhysics::FilterGroup::None);

    // Fire FMOD event
    EngineAudio.setGlobalRTPC(FMOD_PARAM_END, 0);
    EngineAudio.postEvent(FMOD_EVENT);

    // Set FMOD parameter to filter and modify gain
    EngineAudio.setGlobalRTPC(FMOD_PARAM_WARP, 1);

    return true;
}

void TCompTimeReversal::stopRewinding()
{
    // Reset delta scale
    TCompGameManager* c_game_manager = h_game_manager;
    c_game_manager->setTimeStatusLerped(TCompGameManager::ETimeStatus::NORMAL, 0.5f);

    TCompSkeleton* c_skeleton = h_skeleton;
    c_skeleton->custom_bone_update = false;

    // Restore default filters
    TCompCollider* c_collider = h_collider;
    EnginePhysics.setupFilteringOnAllShapesOfActor(c_collider->actor, CModulePhysics::FilterGroup::Player, CModulePhysics::FilterGroup::All);

    // Check if I have passed over a death camera trigger
    {
        CEntity* cam = getEntityByName("camera_follow"); 
        assert(cam);
        TCompCameraFollow* follow = cam->get<TCompCameraFollow>();
        assert(follow);
        bool overDeathCamera = follow->target_dead;
            
        TCompTransform* comp_transform = h_transform;
        VHandles hs;
        overDeathCamera &= EnginePhysics.raycast(comp_transform->getPosition(),
            VEC3::Down, 10.f, hs, CModulePhysics::FilterGroup::Trigger);

        if(!overDeathCamera)
            follow->target_dead = false;
    }

    // Send message to controller
    TMsgTimeReversalStoped msg;
    msg.force_scale = force_scale_timer / (max_seconds * rewind_speed);
    msg.state_name = lerp_target.state->name;
    msg.time_in_anim = lerp_target.time_in_anim;
    msg.linear_velocity = lerp_target.linear_velocity;
    CHandle(this).getOwner().sendMsg(msg);

    // Consume warp slots
    TCompWarpEnergy* warp_energy_comp = get<TCompWarpEnergy>();
    warp_energy_comp->consumeWarpEnergy(getWarpConsumption());

    // Reset internal variables
    is_rewinding = false;
    generated_shots = 0;
    force_scale_timer = 0.0f; 
    stop_timer = 0.f;
    
    // Begin lifetime for Eon copy if created
    CEntity* e_holo = h_holo;
    if (e_holo) {
        TCompLifetime* lTime = e_holo->get<TCompLifetime>();
        lTime->init();

        TCompDissolveEffect* c_dissolve = e_holo->get<TCompDissolveEffect>();
        if (c_dissolve) {
            c_dissolve->setRemoveColliders(true);
            c_dissolve->recover(7.f, 2.4f, true);
        }
    }

    // Set FMOD stop parameter
    EngineAudio.setGlobalRTPC(FMOD_PARAM_END, 1);

    // Set FMOD parameter to filter and modify gain
    EngineAudio.setGlobalRTPC(FMOD_PARAM_WARP, 0);
}

STRShot TCompTimeReversal::interpolatedValue(float lerp_value)
{
    VEC3 position_lerp = lerp(lerp_origin.position, lerp_target.position, lerp_value);
    float yaw_lerp = lerpRadians(lerp_origin.yaw, lerp_target.yaw, 1.0f, lerp_value);

    TCompSkeleton* c_skeleton = h_skeleton;
    for (int bone_idx = 0; bone_idx < bones_size; ++bone_idx) {
        c_skeleton->cb_bones.bones[bone_idx] = MAT44::Lerp(lerp_origin.bones[bone_idx], lerp_target.bones[bone_idx], lerp_value);
    }

    //std::copy(std::begin(lerp_target.bones), std::end(lerp_target.bones), std::begin(c_skeleton->cb_bones.bones));
    c_skeleton->cb_bones.updateFromCPU();

    return { position_lerp, yaw_lerp };
}

void TCompTimeReversal::generateShot()
{
    time_reversal_timer -= TIME_PER_SHOT;
    TCompTransform* transform = h_transform;
    TCompHealth* c_health = h_player_health;
    TCompSkeleton* c_skeleton = h_skeleton;
    TCompFSM* c_fsm = h_fsm;
    TCompCollider* c_collider = h_collider;

    STRShot shot = { transform->getPosition(), vectorToYaw(transform->getForward()), c_health->getHealth() };

    // Add animation data
    shot.state = c_fsm->getCurrentState();
    shot.time_in_anim = c_fsm->getCtx().getTimeInState();
    //shot.linear_velocity = PXVEC3_TO_VEC3(((physx::PxRigidDynamic*)c_collider->force_actor)->getLinearVelocity());

    std::copy(c_skeleton->cb_bones.bones, c_skeleton->cb_bones.bones + bones_size, shot.bones);
    addShot(shot);
}

void TCompTimeReversal::addShot(const STRShot& shot)
{
    circular_buffer[current_index] = shot;
    current_index = (current_index + 1) % buffer_size;

    if (generated_shots < buffer_size) {
        generated_shots++;
    }
}

void TCompTimeReversal::consumeShot()
{
    lerp_origin = circular_buffer[rewind_index];
    rewind_index = rewind_index == 0 ? (buffer_size - 1) : rewind_index - 1;
    lerp_target = circular_buffer[rewind_index];

    TCompPlayerController* c_player = h_player;

    TCompHealth* c_health = h_player_health;
    c_health->setHealth(lerp_target.current_health);

    //dbg("----------- (%f %f %f) - (%f %f %f)\n", lerp_origin.position.x, lerp_origin.position.y, lerp_origin.position.z, lerp_target.position.x, lerp_target.position.y, lerp_target.position.z);

    generated_shots--;
}

void TCompTimeReversal::spawnHolo()
{
    TCompTransform* transform = h_transform;
        
    // Opción sin mirar a nadie usar solo rotacion de Eon
    h_holo = spawn("data/prefabs/eon_holo.json", *transform);
    CEntity* e_holo = h_holo;

    TCompDissolveEffect* c_dissolve = e_holo->get<TCompDissolveEffect>();
    if (c_dissolve) {
        c_dissolve->setRemoveColliders(false);
        c_dissolve->enable(0.25f, 0.001f, true);
    }

    return;

    // --------------------------------------------------
    // Get closest enemy (if any) and rotate the Copy to it
    VHandles objects_in_area;
    CHandle closest;

    if (!EnginePhysics.overlapSphere(transform->getPosition(), 15.f, objects_in_area, CModulePhysics::FilterGroup::Enemy))
        return;

    float min_distance = 100000.0f;

    for (CHandle collider_handle : objects_in_area) {

        CHandle h_owner = collider_handle.getOwner();
        CEntity* entity_object = h_owner;

        if (!entity_object)
            continue;

        CHandle h_health = entity_object->get<TCompHealth>();
        TCompTransform* overlapped_trans = entity_object->get<TCompTransform>();

        if (!overlapped_trans || !h_health.isValid() || static_cast<TCompHealth*>(h_health)->isDead())
            continue;

        TCompHierarchy* c_hierarchy = entity_object->get<TCompHierarchy>();

        if (c_hierarchy) {
            entity_object = c_hierarchy->getRootEntity(entity_object);
        }

        VEC3 overlapped_pos = overlapped_trans->getPosition();

        float distance = VEC3::DistanceSquared(overlapped_pos, transform->getPosition());
        if (distance < min_distance) {
            distance = min_distance;
            closest = entity_object;
        }
    }

    if (closest.isValid())
    {
        CEntity* eClosest = closest;
        float delta_yaw = transform->getYawRotationToAimTo(eClosest->getPosition());
        QUAT q = QUAT::CreateFromAxisAngle(VEC3::Up, delta_yaw);
        CTransform t;
        t.fromMatrix(*transform);
        t.setRotation(t.getRotation() * q);
        spawn("data/prefabs/eon_holo.json", t);
    }
}

void TCompTimeReversal::debugInMenu()
{
    ImGui::Text(is_rewinding ? "REWINDING" : "NOT REWINDING");
    ImGui::ProgressBar(generated_shots / (float)buffer_size, ImVec2(-1, 0));
    ImGui::DragInt("Warp consumption", &warp_consumption, 1, 0, 50);
}

void TCompTimeReversal::renderDebug()
{
    // float max_value = max_seconds * rewind_speed;
    // drawSpinner(VEC2(53, 43), VEC4(1.0), (float)generated_shots, (float)buffer_size, 14, 8);
}