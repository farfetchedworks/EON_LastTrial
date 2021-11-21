#include "mcv_platform.h"
#include "engine.h"
#include "entity/entity_parser.h"
#include "comp_weapon.h"
#include "comp_hitmap.h"
#include "render/render_module.h"
#include "components/common/comp_transform.h"
#include "components/controllers/pawn_utils.h"
#include "components/gameplay/comp_destructible_prop.h"
#include "components/gameplay/comp_eter.h"
#include "components/common/comp_hierarchy.h"
#include "components/controllers/comp_player_controller.h"
#include "modules/module_physics.h"
#include "../bin/data/shaders/constants_particles.h"

bool TCompWeapon::checkHit(CEntity* eSrc, CEntity* eDst, bool& fromBack)
{
    bool hit = PawnUtils::isInsideCone(eSrc, eDst, deg2rad(180.f), range);

    if (hit)
    {
        TCompTransform* tSrc = eSrc->get<TCompTransform>();
        TCompTransform* tDst = eDst->get<TCompTransform>();

        float fwdDot = tSrc->getForward().Dot(tDst->getForward());
        fromBack = fwdDot > 0.85f;
    }

    return hit;
}

void TCompWeapon::onTriggerEnter(const TMsgEntityTriggerEnter& msg, CHandle h_owner, VEC3 position)
{
    CEntity* owner = h_owner;
    CEntity* eHit = msg.h_entity;

    if (!eHit || !enabled || in_area_delay)
        return;

    CEntity* eParent = parent;

    TCompDestructible* destructible = eHit->get<TCompDestructible>();
    TCompEter* eter = eHit->get<TCompEter>();
    if (destructible) 
    {
        TCompTransform* ownerTransform = eParent->get<TCompTransform>();
        TMsgPropDestroyed msg;
        msg.direction = ownerTransform->getForward();
        eHit->sendMsg(msg);
    }
    else if (eter)
    {
        eter->onHit();
    }
    else {

        // Check hit map
        {
            TCompHitmap* hitmap = owner->get<TCompHitmap>();
            if (hitmap) {
                if (!hitmap->resolve(msg.h_entity))
                    return;
            }
        }

        processDamage(parent, msg.h_entity);

        CEntity* target = msg.h_entity;
        TCompHierarchy* c_hierarchy = target->get<TCompHierarchy>();
        if (c_hierarchy) {
            target = c_hierarchy->h_parent_transform.getOwner();
        }

        // Move collision to capsule collision with enemy
        {
            std::vector<physx::PxRaycastHit> raycastHits;
            bool is_ok = EnginePhysics.raycast(position, normVEC3(target->getPosition() + VEC3::Up - position),
                100.f, raycastHits, CModulePhysics::FilterGroup::Enemy, true);
            if (is_ok) {
                position = PXVEC3_TO_VEC3(raycastHits[0].position);
            }
        }

        // Move towards enemy
        {
            VEC3 collisionToTarget = normVEC3((target->getPosition() + VEC3::Up) - position);
            position += collisionToTarget * 0.3f;
        }

        TEntityParseContext ctx;
        CTransform t;
        t.setPosition(position);

        if (CHandle(target) == getEntityByName("Gard")) {

            spawn("data/particles/gard_hit_particles.json", t, ctx);

            for (auto h : ctx.entities_loaded) {
                CEntity* e = h;
                TCompBuffers* buffers = e->get<TCompBuffers>();
                CShaderCte< CtesParticleSystem >* cte = static_cast<CShaderCte< CtesParticleSystem >*>(buffers->getCteByName("CtesParticleSystem"));
                cte->emitter_initial_pos = position + Random::vec3() * 0.2f;
                cte->emitter_owner_position = target->getPosition();
                cte->updateFromCPU();

                TCompTransform* hTrans = e->get<TCompTransform>();
                hTrans->fromMatrix(MAT44::CreateRotationY(Random::range((float)-M_PI, (float)M_PI)) *
                    MAT44::CreateTranslation(position));
            }
        }
        else
        {
            // Hit too high..
            if (fabsf(position.y - target->getPosition().y) > 1.8f)
                return;

            // Blood Trail
            TCompPlayerController* controller = eParent->get<TCompPlayerController>();
            if (!controller) return;

            TCompTransform* trans_target = target->get<TCompTransform>();
            VEC3 tPos = trans_target->getPosition() + VEC3(0, 1.25f, 0);

            // Detect Hit Type to get trail direction

            std::string particlesName = "";

            if (std::get<bool>(controller->getVariable("is_sprint_regular_attack"))) {
                particlesName = "data/particles/splatter_blood_left.json";
            }
            else if (std::get<bool>(controller->getVariable("is_sprint_strong_attack")) || 
                controller->is_dash_strike) {
                particlesName = "data/particles/splatter_blood_front.json";
            }
            else {
                int attackHeavy = std::get<int>(controller->getVariable("is_attacking_heavy"));

                if (attackHeavy > 0)
                {
                    switch (attackHeavy)
                    {
                        // Vertical (up to down)
                        case 1:
                            particlesName = "data/particles/splatter_blood_front.json";
                            break;
                        // Charge (left to right)
                        case 2:
                            particlesName = "data/particles/splatter_blood_left.json";
                            break;
                    }
                }
                else {
                    switch (controller->attack_count) {
                        case 1:
                        case 3:
                            particlesName = "data/particles/splatter_blood_left.json";
                            break;
                        case 2:
                        case 4:
                            particlesName = "data/particles/splatter_blood_right.json";
                            break;
                    }
                }
            }

            if(particlesName.length())
                spawnParticles(particlesName, tPos, tPos);
        }
    }
}