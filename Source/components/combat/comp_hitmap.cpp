#include "mcv_platform.h"
#include "engine.h"
#include "comp_hitmap.h"
#include "components/common/comp_transform.h"
#include "components/common/comp_hierarchy.h"

DECL_OBJ_MANAGER("hitmap", TCompHitmap);

void TCompHitmap::onEntityCreated()
{
    
}

bool TCompHitmap::resolve(CHandle h)
{
    assert(h.isValid());

    CEntity* e = h;
    CEntity* eHit = nullptr;

    // Get owner entity, parent in case of weapons/gard colliders
    TCompHierarchy* c_hierarchy = e->get<TCompHierarchy>();
    if (c_hierarchy) {
        CEntity* eParent = c_hierarchy->h_parent_transform.getOwner();
        eHit = eParent;
    }
    else {
        eHit = h;
    }

    if (!eHit)
        return false;

    uint32_t index = CHandle(eHit).getExternalIndex();
    auto it = m_canHit.find(index);

    // We have the handle, if I hit before, don't hit 
    // Else, update the hit info
    if (it != m_canHit.end()) {
        if (!it->second.allowed)
            return false;
        else 
            it->second.allowed = false;
    }
    // New handle, we register it with allow=false
    else {
        TCharacterHitInfo info;
        info.h = CHandle(eHit);
        info.allowed = false;
        m_canHit[index] = info;
    }

    return true;
}

void TCompHitmap::reset()
{
    m_canHit.clear();
}

void TCompHitmap::debugInMenu()
{
    for (auto& i : m_canHit) {
        CEntity* e = i.second.h;
        ImGui::Text(e->getName());
        ImGui::Text("Handle: %d", i.first);
        ImGui::Checkbox("Can Hit", &i.second.allowed);
    }
}