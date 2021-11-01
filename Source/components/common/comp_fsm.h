#pragma once

#include "comp_base.h"
#include "entity/entity.h"
#include "fsm/fsm_context.h"

class CGameCoreSkeleton;
struct TMsgFSMVariable;

class TCompFSM : public TCompBase
{
    DECL_SIBLING_ACCESS();

public:
    static void registerMsgs();

    void load(const json& j, TEntityParseContext& ctx);
    void onEntityCreated();
    void update(float dt);
    void debugInMenu();
    void renderDebug();

    void onSetVariable(const TMsgFSMVariable& msg);
    const fsm::IState* getCurrentState();
    fsm::CContext& getCtx() { return _context; };

    static const CGameCoreSkeleton* _currentCoreSkeleton;
    static const CGameCoreSkeleton* getCurrentCoreSkeleton() { return _currentCoreSkeleton; };

private:
    std::string     _name;
    fsm::CContext   _context;
};
