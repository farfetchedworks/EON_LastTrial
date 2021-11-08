#include "mcv_platform.h"
#include "engine.h"
#include "comp_player_weapon.h"
#include "comp_weapon_part_trigger.h"
#include "comp_hitmap.h"
#include "entity/entity_parser.h"
#include "modules/module_physics.h"
#include "components/common/comp_transform.h"
#include "components/common/comp_collider.h"
#include "components/stats/comp_attributes.h"
#include "components/common/comp_hierarchy.h"
#include "components/controllers/comp_pawn_controller.h"
#include "skeleton/comp_attached_to_bone.h"

DECL_OBJ_MANAGER("player_weapon", TCompPlayerWeapon)

void TCompPlayerWeapon::load(const json& j, TEntityParseContext& ctx)
{
    assert(j.count("damages"));
    assert(j["damages"].is_array());
    assert(j["damages"].size() == NUM_ATTACKS);

    for (int i = 0; i < j["damages"].size(); ++i) {
        damages[i] = j["damages"][i];
    }

    range = j.value("range", range);
}

void TCompPlayerWeapon::update(float dt) {}

void TCompPlayerWeapon::onEntityCreated()
{ 
    TCompHierarchy* c_hierarchy = get<TCompHierarchy>();
    CEntity* eParent = c_hierarchy->h_parent_transform.getOwner();
    eParent->sendMsg(TMsgRegisterWeapon({ CHandle(this).getOwner() }));

    owner_weapon = CHandle(this).getOwner();
    parent = eParent;
}

int TCompPlayerWeapon::getDamage(bool fromBack)
{
    CEntity* eParent = parent;

    TCompAttributes* attrs = eParent->get<TCompAttributes>();
    assert(attrs);
    int dmg = attrs->computeDamageSent(damages[current_action]);

    //if (fromBack) dmg = attrs->computeDamageSent(damages[6]);     // 6 is a critical attack (backstab)
    return dmg;
}

void TCompPlayerWeapon::processDamage(CHandle src, CHandle dst)
{
    assert(src.isValid());
    assert(dst.isValid());
    CEntity* eSrc = src;
    CEntity* eDst = dst;
    
    bool fromBack = false;
    bool hit = checkHit(eSrc, eDst, fromBack);

    if (!hit)
        return;

    int dmg = getDamage(fromBack);

    // Manage backstab
    // For now, the only consequence of a backstab is a critical hit, so these lines are commented
    //if (fromBack) {
    //    processBackStab(dst);
    //}

    // Create msg to TCompHealth to reduce health
    {
        TMsgHit msgHit;
        msgHit.damage = dmg;
        msgHit.hitByPlayer = true;
        msgHit.h_striker = src;
        msgHit.fromBack = fromBack;
        eDst->sendMsg(msgHit);

        TMsgAddForce msgForce;
        TCompTransform* tSrc = eSrc->get<TCompTransform>();
        msgForce.force = tSrc->getForward() * 0.05f * (float)dmg;
        msgForce.byPlayer = true;
        msgForce.h_applier = src;
        eDst->sendMsg(msgForce);
    }

    // Create msg to TCompWarp to increase warp on player hit if enemy is not in area delay
    TCompPawnController* enemyController = eDst->get<TCompPawnController>();
    if(enemyController && !enemyController->inAreaDelay()) {
        TMsgHitWarpRecover msgHitWarp;
        msgHitWarp.hitByPlayer = true;
        msgHitWarp.fromBack = fromBack;
        eSrc->sendMsg(msgHitWarp);
    }
}

void TCompPlayerWeapon::processBackStab(CHandle dst)
{
    assert(dst.isValid());
    CEntity* eDst = dst;
    // Stun enemy
    {
         TMsgEnemyStunned msg;
         eDst->sendMsg(msg);
    }
}

void TCompPlayerWeapon::onTriggerEnter(const TMsgEntityTriggerEnter& msg)
{
    // Compute hit position
    CTransform t;

    if (!h_blade_attached_to_bone.isValid()) {
        CEntity* bladeCollider = getEntityByName("Espasa blade collider");
        assert(bladeCollider);
        h_blade_attached_to_bone = bladeCollider->get<TCompAttachedToBone>();
        h_blade_weapon_trigger = bladeCollider->get<TCompWeaponPartTrigger>();
    }

    TCompAttachedToBone* c_attached_to_bone = h_blade_attached_to_bone;
    TCompWeaponPartTrigger* weaponTrigger = h_blade_weapon_trigger;
    c_attached_to_bone->applyOffset(t, weaponTrigger->getOffset());
    // -----------------------

    TCompWeapon::onTriggerEnter(msg, getEntity(), t.getPosition());
}

void TCompPlayerWeapon::onApplyAreaDelay(const TMsgWeaponInAreaDelay& msg)
{
    in_area_delay = msg.in_area_delay;
}

void TCompPlayerWeapon::onSetActive(const TMsgEnableWeapon& msg)
{
	enabled = msg.enabled;
    current_action = msg.action;
    static_cast<TCompCollider*>(get<TCompCollider>())->disable(!enabled);

    // Allow hitting everyone in each new attack
    CEntity* owner = owner_weapon;
    TCompHitmap* hitmap = owner->get<TCompHitmap>();
    if (hitmap && enabled) {
        hitmap->reset();
    }
}

void TCompPlayerWeapon::debugInMenu()
{
    ImGui::DragInt("Regular Dmg", &dmg_regular, 1, 0, 100);
    ImGui::DragInt("Heavy Dmg", &dmg_heavy, 1, 0, 100);
}