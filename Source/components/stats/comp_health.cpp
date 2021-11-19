#include "mcv_platform.h"
#include "engine.h"
#include "comp_health.h"
#include "entity/entity.h"
#include "render/render.h"
#include "render/draw_primitives.h"
#include "components/messages.h"
#include "components/common/comp_name.h"
#include "components/common/comp_transform.h"
#include "components/controllers/comp_player_controller.h"
#include "components/cameras/comp_camera_follow.h"
#include "components/stats/comp_geons_drop.h"
#include "modules/game/module_gameplay.h"
#include "components/gameplay/comp_game_manager.h"
#include "components/common/comp_parent.h"
#include "ui/ui_module.h"
#include "ui/widgets/ui_image.h"
#include "components/ai/comp_bt.h"
#include "bt/bt_context.h"
#include "bt/bt_blackboard.h"

DECL_OBJ_MANAGER("health", TCompHealth)

void TCompHealth::load(const json& j, TEntityParseContext& ctx)
{
    assert(j["max_health"].is_number());
    max_health = j.value("max_health", max_health);

    // Get JSON's start health or start with max health by default
    health = j.value("start_health", max_health);
    lerp_health = health;

    is_boss = j.value("is_boss", is_boss);
    render_active = j.value("render_active", !is_boss);
    interpolator = &interpolators::cubicInInterpolator;
}

void TCompHealth::onEntityCreated()
{
    if (CHandle(this).getOwner() == getEntityByName("player")){
        is_player = true;
        return;
    }

    // Reduce the enemy's max health according to the DDA parameter correction
    if (GameManager)
    {
        TCompGameManager* c_game_manager = GameManager->get<TCompGameManager>();
        max_health = static_cast<int>(c_game_manager->getAdjustedParameter(TCompGameManager::EParameterDDA::MAX_HP, (float)max_health));
        health = max_health;
        lerp_health = max_health;
    }
}

void TCompHealth::update(float dt)
{
    elapsedHitTime += dt;
    debugTimer += dt;
    lerpHealth(dt);

    if (!is_player)
        return;

    // Update UI for PLAYER
    ui::CWidget* w = EngineUI.getWidgetFrom("eon_hud", "life_bar");
    assert(w);

    ui::CWidget* wChild = w->getChildByName("bar_fill");
    if (wChild) {
        ui::CImage* fill = static_cast<ui::CImage*>(wChild);
        ui::TImageParams& params = fill->imageParams;
        params.alpha_cut = (float)health / (float)max_health;
    }

    wChild = w->getChildByName("bar_fill_lerp");
    if (wChild) {
        ui::CImage* fill = static_cast<ui::CImage*>(wChild);
        ui::TImageParams& params = fill->imageParams;
        params.alpha_cut = (float)lerp_health / (float)max_health;
    }

    /*wChild = w->getChildByName("bar_background");
    if (wChild) {
        ui::CImage* fill = static_cast<ui::CImage*>(wChild);
        ui::TImageParams& params = fill->imageParams;
        params.alpha_cut = (float)lerp_health / (float)max_health;
    }*/
}

void TCompHealth::debugInMenu()
{
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, (ImVec4)ImColor::ImColor(0.11f, 0.8f, 0.15f));
    ImGui::ProgressBar(health / 100.f, ImVec2(-1, 0));
    ImGui::PopStyleColor();
}

bool TCompHealth::checkDeathAnimationCompleted() {

    bool completed = death_animation_completed;

    if (!completed)
        death_animation_completed = true;

    return completed;
}

/*
  Reduce HP when receiving a hit from an enemy
  Remember to add armor to calculations
*/
void TCompHealth::onReduceHealth(const TMsgReduceHealth& msg)
{
    int damage = msg.damage;
    CEntity* e_striker = msg.h_striker;

    dbg("Damage dealt: %d\n", damage);
    reduceHealth(damage);

    // If health reduced on player
    if (!msg.hitByPlayer && !msg.skip_blood) {
        TCompTransform* c_trans = get<TCompTransform>();
        VEC3 position = c_trans->getPosition() + VEC3(0, 1.25f, 0);
        spawnParticles("data/particles/splatter_blood_front.json", position, position);
    }

    debugTimer = 0.f;
    elapsedHitTime = 0.f;

    // If the player has hit and health = 0, send a message to the GeonsManager
    if (health != 0)
        return;

    // Manage enemy death
    if (msg.hitByPlayer)
    {
        TCompGeonsDrop* comp_geons_drop = get<TCompGeonsDrop>();

        // This should never happen
        if (!CHandle(comp_geons_drop).isValid())
            return;

        TCompParent* c_parent = get<TCompParent>();

        // disable the collider of my children
        if (CHandle(c_parent).isValid())
            c_parent->disableChildCollider();

        TMsgEnemyDied msgEnemyDied;
        msgEnemyDied.geons = comp_geons_drop->getGeonsDropped();
        msgEnemyDied.h_enemy = CHandle(this).getOwner();
        msgEnemyDied.backstabDeath = msg.fromBack;
        CEntity* e_player = msg.h_striker;
        e_player->sendMsg(msgEnemyDied);

        CEntity* e_owner = CHandle(this).getOwner();
        TCompBT* c_bt = e_owner->get<TCompBT>();
        //c_bt->getContext()->setCurrentNode(nullptr);

        // Reset BT context
        c_bt->getContext()->reset();
    }
    // Manage EON death
    else
    {
        TMsgEonHasDied msgEonDied;
        msgEonDied.h_sender = CHandle(this);
        GameManager->sendMsg(msgEonDied);
    }
}

void TCompHealth::setHealth(int value)
{
    health = value;

    if (health <= 0 && is_player) {
      TMsgEonHasDied msgEonDied;
      msgEonDied.h_sender = CHandle(this);
      GameManager->sendMsg(msgEonDied);
    }
}

void TCompHealth::fillHealth()
{
    health = curr_max_health;
    lerp_health = curr_max_health;

    dbg("Current health %d\n", health);
}

void TCompHealth::increaseHealth(int health_points)
{
    health = std::min<int>(health + health_points, curr_max_health);
    lerp_health = health;
}

void TCompHealth::reduceHealth(int health_points)
{
    TCompGameManager* gm = GameManager->get<TCompGameManager>();

    if (!gm->isEonDied())
    {
        lastDamageTaken = health_points;
        health = std::max<int>(health - health_points, 0);
    }
}

void TCompHealth::lerpHealth(float dt)
{
    if (elapsedHitTime >= lerpHealthWaitTime) {
        lerp_health = damp<int>(lerp_health, health, 1.f, dt);
    }
}

bool TCompHealth::isDead() {
    return (health <= 0);
}

void TCompHealth::renderDebug() {

    if (isDead() && !is_player)
        return;

#ifdef _DEBUG

    if (debugTimer < damageDebugTime) {
        TCompTransform* trans = get<TCompTransform>();
        VEC3 pos = trans->getPosition() + VEC3(0, 1.8f, 0);
        pos.y += debugTimer;
        drawText3D(pos, Colors::Black, std::to_string(lastDamageTaken).c_str(), 25.f);
    }

#endif

    if (!render_active)
        return;

    TCompName* c_name = get<TCompName>();

    float pct = health / (float)max_health;
    Color clr = pct > 0.333 ? (pct > 0.666 ? Colors::Green : Colors::Orange) : Colors::Red;

    if (is_player) {
        
        /*drawProgressBar2D(VEC2(30, 70), clr, (float)health, (float)max_health, "health", 160, 
            20.f, true, (float)lerp_health, clr * 0.8f);*/
    }
    else
    {
        if (is_boss)
        {
            float width = (float)Render.getWidth();
            float sizeW = 500.f;
            drawProgressBar2D(VEC2(width/2.f - sizeW/2.f, 40), clr, (float)health, (float)max_health, nullptr, sizeW, 20.f);
            drawText2D(VEC2(-1.f, 65), Colors::White, c_name->getName(), true);
        }
        else
        {
            TCompCameraFollow* c_follow = static_cast<CEntity*>(getEntityByName("camera_follow"))->get<TCompCameraFollow>();
            if (health == max_health && c_follow->h_trans_locked_enemy.getOwner() != CHandle(this).getOwner()) {
                return;
            }

            TCompTransform* trans = get<TCompTransform>();
            VEC3 pos = trans->getPosition();
            drawProgressBar3D(pos, clr, (float)health, (float)max_health);
        }
    }
}
