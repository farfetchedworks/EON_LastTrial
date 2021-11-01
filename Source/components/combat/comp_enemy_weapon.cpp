#include "mcv_platform.h"
#include "engine.h"
#include "comp_enemy_weapon.h"
#include "comp_hitmap.h"
#include "modules/module_physics.h"
#include "components/common/comp_transform.h"
#include "components/common/comp_collider.h"
#include "components/common/comp_render.h"
#include "components/controllers/pawn_actions.h"
#include "skeleton/comp_attached_to_bone.h"
#include "components/common/comp_hierarchy.h"

DECL_OBJ_MANAGER("enemy_weapon", TCompEnemyWeapon)

void TCompEnemyWeapon::load(const json& j, TEntityParseContext& ctx) {
    dmg_regular = j.value("dmg_regular", dmg_regular);
    dmg_heavy = j.value("dmg_heavy", dmg_heavy);
    range = j.value("range", range);
    hit_type = j.value("hit_type", std::string());
}

void TCompEnemyWeapon::update(float dt) {}

void TCompEnemyWeapon::onEntityCreated()
{
    TCompAttachedToBone* socket = get<TCompAttachedToBone>();

    // Get the transform of the parent to get the actual owner
    TCompHierarchy* c_hierarchy = get<TCompHierarchy>();
    CEntity* eParent = c_hierarchy->h_parent_transform.getOwner();        // This was here before getEntityByName(socket->target_entity_name);
    eParent->sendMsg(TMsgRegisterWeapon({ CHandle(this).getOwner() }));

    owner_weapon = CHandle(this).getOwner();
    parent = eParent;
}

void TCompEnemyWeapon::processDamage(CHandle src, CHandle dst)
{
    assert(src.isValid());
    assert(dst.isValid());
    CEntity* eSrc = src;
    CEntity* eDst = dst;
    TCompTransform* tSrc = eSrc->get<TCompTransform>();

    // testing hits with inside cone
    bool fromBack = false;
    bool hit = checkHit(eSrc, eDst, fromBack);
    if (!hit)
        return;

    int dmg = current_damage;

    // Create msg to TCompHealth to reduce health
    {
        TMsgHit msgHit;
        msgHit.damage = dmg;
        msgHit.hitByPlayer = false;
        msgHit.h_striker = src;

        if(hit_type == "sweep")
            msgHit.hitType = EHIT_TYPE::SWEEP;
        if (hit_type == "smash")
            msgHit.hitType = EHIT_TYPE::SMASH;
        
        eDst->sendMsg(msgHit);

       /* TMsgAddForce msgForce;
        msgForce.force = tSrc->getForward() * 0.08f * (float)dmg;
        msgForce.byPlayer = false;
        msgForce.h_applier = src;
        eDst->sendMsg(msgForce);*/
    }
}

void TCompEnemyWeapon::onTriggerEnter(const TMsgEntityTriggerEnter& msg)
{
    TCompWeapon::onTriggerEnter(msg, getEntity(), VEC3::Zero);
}

void TCompEnemyWeapon::onApplyAreaDelay(const TMsgWeaponInAreaDelay& msg)
{
    in_area_delay = msg.in_area_delay;
}

void TCompEnemyWeapon::onSetActive(const TMsgEnableWeapon& msg)
{
    enabled = msg.enabled;
    current_damage = msg.cur_dmg;
    static_cast<TCompCollider*>(get<TCompCollider>())->disable(!enabled);

    // Allow hitting everyone in each new attack
    CEntity* owner = owner_weapon;
    TCompHitmap* hitmap = owner->get<TCompHitmap>();
    if (hitmap && enabled) {
        hitmap->reset();
    }
}

void TCompEnemyWeapon::debugInMenu()
{
    ImGui::DragInt("Regular Dmg", &dmg_regular, 1, 0, 100);
    ImGui::DragInt("Heavy Dmg", &dmg_heavy, 1, 0, 100);
}