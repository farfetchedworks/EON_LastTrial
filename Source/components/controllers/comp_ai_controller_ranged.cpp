#include "mcv_platform.h"
#include "comp_ai_controller_ranged.h"

/*

void TCompAIControllerRanged::load(const json& j, TEntityParseContext& ctx)
{
    TCompAIControllerBase::loadCommonProperties(j);

    // Load properties specific to ranged enemy
    oscillate_speed = j.value("oscillate_speed", oscillate_speed);

    ranged_attack_range = j.value("ranged_attack_range", ranged_attack_range);
    ranged_attack_dmg = j.value("ranged_attack_dmg", ranged_attack_dmg);
}

void TCompAIControllerRanged::onEntityCreated()
{
    TCompAIControllerBase::onEntityCreatedCommon(getEntity());

    // Load state timings
    loadTimingsFromJson("data/actions/AI_ranged_timings.json");
}

void TCompAIControllerRanged::debugInMenu()
{
    debugCommonProperties();

    // Debug properties specific to melee1 enemy
    ImGui::Separator();

    ImGui::DragFloat("Oscillate Speed", &oscillate_speed, 0.2f, 0.f, 60.f);

    ImGui::DragFloat("Ranged Attack Range", &ranged_attack_range, 0.1f, 0.f, 600.f);
    ImGui::DragInt("Ranged Attack Damage", &ranged_attack_dmg, 1, 0, 100);

    ImGui::Separator();

    ImGui::Text("Current speed (after multiplier): %f", (double)move_speed * speed_multiplier);
}

void TCompAIControllerRanged::update(float dt)
{
    TCompAIControllerBase::update(dt);
}
*/

/*


void TCompAIControllerRanged::hit(CEntity* entity, const int dmg)
{
    assert(entity);

    TMsgHit msgHit;
    msgHit.damage = dmg;
    msgHit.h_striker = CHandle(this).getOwner();

    entity->sendMsg(msgHit);
}

void TCompAIControllerRanged::combatDecision()
{
    // Retrieve Eon's distance and enemy health
    TCompHealth* health_comp = enemy_health;
    const bool enemy_low_health = health_comp->getHealth() <= TH_enable_heal;

    // If health is low and %chance to heal, heal
    if (enemy_low_health && uniform_0_to_100(*rand_gen) <= chance_heal) {
        changeState("healing");
    }
    else {
        consecutive_attacks_left = 1 + uniform_0_to_100(*rand_gen) % 2;
        chainAttackDecision();
    }
}


void TCompAIControllerRanged::chainAttackDecision()
{
    if (consecutive_attacks_left > 0) {
        // allow only normal attacks
        consecutive_attacks_left--;
        changeState("range_attack");
    }
    else {
        // attack chain has finished
        changeState("idle_war");
    }
}

*/

//////////////////////////////////////////////////////////////////////
/////////////////////// UTILITY FUNCTIONS ////////////////////////////
//////////////////////////////////////////////////////////////////////

/*

void TCompAIControllerRanged::oscillate(const float dt)
{
    TCompTransform* t = enemy_transform;

    // Calculate radians to rotate
    float rad_to_rotate = deg2rad(10.f) * oscillate_speed * dt;
    if (!oscillate_left) {
        rad_to_rotate = -rad_to_rotate;
    }

    oscillate_rad_acum += rad_to_rotate;

    // Apply rotation
    QUAT delta_rotation = QUAT::CreateFromAxisAngle(VEC3(0, 1, 0), rad_to_rotate);
    QUAT new_rotation = t->getRotation() * delta_rotation;
    t->setRotation(new_rotation);

    // Check if oscillation thresholds have been surpassed, to swap rotation
    if (oscillate_left && oscillate_rad_acum > MAX_OSCILLATION_RAD) {
        oscillate_left = !oscillate_left;
    }
    else if (!oscillate_left && oscillate_rad_acum < -MAX_OSCILLATION_RAD) {
        oscillate_left = !oscillate_left;
    }
}

*/

//////////////////////////////////////////////////////////////////////
/////////////////////////// STATES ///////////////////////////////////
//////////////////////////////////////////////////////////////////////

/*

void TCompAIControllerRanged::observing(float dt)
{
    enableActionMesh(EAction::IDLE);

    // To apply gravity
    movePhysics(VEC3::Zero, dt);

    // Oscilate sight
    oscillate(dt);

    // Check if Eon is seen
    CHandle player = getEntityByName("player");
    if (PawnUtils::isInsideCone(CHandle(this).getOwner(), player, fov_rad, sight_radius)) {
        changeState("chase_and_warn");
        oscillate_rad_acum = 0.f;
    }
}

void TCompAIControllerRanged::chaseAndWarn(float dt)
{
    enableActionMesh(EAction::MOVE);

    //TODO: Alert the AI Brain so that nearby melees also chase Eon

    TCompTransform* t = enemy_transform;

    CEntity* player = getEntityByName("player");
    VEC3 player_pos = player->getPosition();

    // Rotate accordingly to face Eon
    rotateToFace(player_pos, rotation_speed, dt);

    // Advance towards Eon
    move(move_speed, dt);

    // If Eon is close enough, transition to idle war
    // Else if Eon is way too far, transition to return to origin
    const float dist = VEC3::Distance(t->getPosition(), player_pos);

    if (dist < TH_idle_war) {
        changeState("idle_war");
    }
    else if (dist > TH_chase_player * 1.75f) {
        changeState("return_to_origin");
    }
}

void TCompAIControllerRanged::returnToOrigin(float dt)
{
    enableActionMesh(EAction::MOVE);

    TCompTransform* t = enemy_transform;

    // Regen HP, since Eon is forgotten
    static float heal_period_checker = 0.f;
    heal_period_checker += dt;

    if (heal_period_checker * 1000 > RETURN_HEALING_PERIOD_MS) {
        TCompHeal* h = enemy_heal;
        h->heal();

        heal_period_checker = 0.f;
    }

    // Face original spawnpoint
    rotateToFace(origin, rotation_speed, dt);

    // Move towards it
    move(move_speed, dt);

    // If target destination is close enough, transition
    const float dist = VEC3::Distance(t->getPosition(), origin);

    if (dist < TH_reach_target) {
        changeState("reset_forward");

        heal_period_checker = 0.f;
    }
}

void TCompAIControllerRanged::resetForward(float dt)
{
    TCompTransform* t = enemy_transform;

    // To apply gravity
    movePhysics(VEC3::Zero, dt);

    // Rotate to reach original observing orientation
    rotate(oscillate_speed, dt);

    float cur_yaw;
    float cur_pitch;

    t->getEulerAngles(&cur_yaw, &cur_pitch, nullptr);

    if (cur_yaw > -0.01f && cur_yaw < 0.01f) {
        changeState("observing");
    }

    // Check if Eon is seen
    CHandle player = getEntityByName("player");
    if (PawnUtils::isInsideCone(CHandle(this).getOwner(), player, fov_rad, sight_radius)) {
        changeState("chase_and_warn");
    }
}

void TCompAIControllerRanged::idleWar(float dt)
{
    enableActionMesh(EAction::IDLE);

    TCompTransform* t = enemy_transform;

    // To apply gravity
    movePhysics(VEC3::Zero, dt);

    SActionCallbacks callbacks;
    CEntity* player = getEntityByName("player");

    // Combat movements
    callbacks.onActive = [this, player, dt]() {
        rotateToFace(player->getPosition(), rotation_speed, dt);
    };

    // Take a combat decision after wait period has elapsed
    callbacks.onRecoveryFinished = [this]()
    {
        combatDecision();
    };

    doActionInTime(callbacks, action_timings[EAction::IDLE_WAR], dt);

    const float dist = VEC3::Distance(t->getPosition(), player->getPosition());
    // If Eon is way too far, return to origin
    if (dist > TH_chase_player) {
        changeState("return_to_origin");
    }
    // If Eon is far, chase him
    else if (dist < TH_chase_player && dist > TH_idle_war + 2) {
        changeState("chase_and_warn");
    }
}

void TCompAIControllerRanged::rangeAttack(float dt)
{
    enableActionMesh(EAction::RANGE_ATTACK);

    // To apply gravity
    movePhysics(VEC3::Zero, dt);
    
    SActionCallbacks callbacks;

    callbacks.onFirstFrame = [this]()
    {
        CEntity* player = getEntityByName("player");
        _lastPlayerPos = player->getPosition();
    };

    callbacks.onStartup = [this, dt]()
    {
        CEntity* player = getEntityByName("player");
        rotateToFace(player->getPosition(), rotation_speed * 0.4f, dt);
    };

    callbacks.onActiveFinished = [this]()
    {
        // Generate raycast proyectile
        TCompTransform* t = enemy_transform;

        // Check if Eon overlaps with attack sphere AROUND enemy
        physx::PxRaycastHit hit_result;
        physx::PxQueryFlags q_flags;
        physx::PxQueryFilterData q_filter;
        q_filter.flags |= physx::PxQueryFlag::eDYNAMIC;

        VHandles colliders;
        CEntity* player = getEntityByName("player");
        VEC3 shoot_orig = t->getPosition() + t->getForward() + VEC3(-0.25f, 0.75f, 0);
        float random_height = uniform_0f_to_1f(*rand_gen);
        VEC3 shoot_dest = player->getPosition();
        shoot_dest += VEC3(0, random_height, 0);

        // Spawn projectile
        spawnProjectile(shoot_orig, shoot_dest, ranged_attack_dmg, false);
    };

    callbacks.onRecoveryFinished = [this]()
    {
        // Attack has finished
        chainAttackDecision();
        enableActionMesh(EAction::IDLE);
    };

    doActionInTime(callbacks, action_timings[EAction::RANGE_ATTACK], dt);
}

void TCompAIControllerRanged::healing(float dt)
{
    enableActionMesh(EAction::HEAL);

    static float heal_period_checker = 0.f;
    heal_period_checker += dt;

    // To apply gravity
    movePhysics(VEC3::Zero, dt);

    SActionCallbacks callbacks;

    callbacks.onActive = [this]()
    {
        if (heal_period_checker * 1000 > COMBAT_HEALING_PERIOD_MS) {
            TCompHeal* h = enemy_heal;
            h->heal();

            heal_period_checker = 0.f;
        }
    };

    callbacks.onRecoveryFinished = [this]()
    {
        changeState("idle_war");
        heal_period_checker = 0.f;
    };

    doActionInTime(callbacks, action_timings[EAction::HEAL], dt);
}

void TCompAIControllerRanged::dodgingHit(float dt)
{
    enableActionMesh(EAction::DASH);

    SActionCallbacks callbacks;

    callbacks.onStartup = [this, dt]()
    {
        dodge(dt);
    };

    callbacks.onRecoveryFinished = [this, dt]()
    {
        changeState("idle_war");
    };

    doActionInTime(callbacks, action_timings[EAction::DASH], dt);
}

void TCompAIControllerRanged::hitstunned(float dt)
{
    enableActionMesh(EAction::RECEIVE_DMG);

    // To apply gravity
    movePhysics(VEC3::Zero, dt);

    // Simulate animation time
    if (sec_inside_state * 1000 > last_hit_hitstun_ms) {
        changeState("idle_war");
    }
}

*/

//////////////////////////////////////////////////////////

/*



void TCompAIControllerRanged::onPreHit(const TMsgPreHit& msg) {

    bool isCancellable = statemap[current_state].isCancellableOnDecision();

    if (isCancellable && msg.hitByPlayer) {
        if (uniform_0_to_100(*rand_gen) <= chance_dodge) {
            changeState("dodgingHit");
        }
    }
}

void TCompAIControllerRanged::onHit(const TMsgHit& msg) {

    // TODO: more hitstun time when attack was heavy...

    bool isCancellable = statemap[current_state].isCancellableOnHit();

    if (isCancellable) {
        last_hit_hitstun_ms = 800;
        changeState("hitstunned");
    }

    // Reduce health
    TMsgReduceHealth hmsg;
    hmsg.damage = msg.damage;
    hmsg.h_striker = msg.h_striker;
    hmsg.hitByPlayer = msg.hitByPlayer;
    CHandle(this).getOwner().sendMsg(hmsg);
}

*/