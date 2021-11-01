#pragma once

struct TEntityParseContext;

struct TCompBase {
  void debugInMenu() {}
  void renderDebug() {}
  void load(const json& j, TEntityParseContext& ctx) {}
  void update(float dt) {}
  void updatePreMultithread(float dt) {}
  void updateMultithread(float dt) { update(dt); }
  void updatePostMultithread(float dt) {}
  void onEntityCreated() {}

  static void registerMsgs() {}
};

// Add this macro inside each derived class so the
// CHandle(this) finds the getObjectManager<TCompConcrete>()
// rather than the getObjectManager<TCompBase>() which does NOT exists
#define DECL_SIBLING_ACCESS()   \
  template< typename TComp >    \
  CHandle get() {                            \
    CEntity* e = CHandle(this).getOwner();   \
    if (!e)                                  \
      return CHandle();                      \
    return e->get<TComp>();                  \
  }  \
  CEntity* getEntity() {   \
    CEntity* e = CHandle(this).getOwner();   \
    return e;  \
  }  \

