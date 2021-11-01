#pragma once

#include "handle/handle.h"
#include "components/common/comp_base.h"

// Before the CEntity because CEntity::sendMsg template needs to know about the all_registered_msgs
#include "callback.h"

class CEntity : public TCompBase {

  CHandle comps[CHandle::max_types];

public:
  ~CEntity();

  CHandle get(uint32_t comp_type) const {
    assert(comp_type < CHandle::max_types);
    return comps[comp_type];
  }

  template< typename TComp >
  CHandle get() const {
    auto om = getObjectManager<TComp>();
    assert(om);
    return comps[om->getType()];
  }

  void debugInMenu();
  void renderDebug();

  void set(uint32_t comp_type, CHandle new_comp);
  void set(CHandle new_comp);
  void load(const json& j, TEntityParseContext& ctx);
  void onEntityCreated();

  void destroy();

  void setName(const char* new_name);
  const char* getName() const;
  
  void setTransform(const CTransform& t, bool is_controller = false);
  void setPosition(const VEC3 pos, bool is_controller = false);
  VEC3 getPosition() const;
  VEC3 getForward() const;
  VEC3 getUp() const;

  template< class TMsg >
  void sendMsg(const TMsg& msg) {

    // Get access to all handlers of that msg type, using the msg as identifier
    auto range = all_registered_msgs.equal_range(TMsg::getMsgID());
    while (range.first != range.second) {
      const auto& slot = range.first->second;

      // If I own a valid component of that type (int)...
      CHandle h_comp = comps[slot.comp_type];
      if (h_comp.isValid())
        slot.callback->sendMsg(h_comp, &msg);

      ++range.first;
    }
  }

  template< typename TComp >
  bool removeComponent() {
      CHandle comp = get<TComp>();
      if (!comp.isValid())
          return false;
      comp.destroy();
      return true;
  }

  template< typename TComp >
  TComp* addComponent() {
      CHandle new_comp = get<TComp>();

      if (new_comp.isValid()) {
          return nullptr;
      }

      new_comp.create<TComp>();
      set(new_comp.getType(), new_comp);
      assert(new_comp.isValid());
      return static_cast<TComp*>(new_comp);
  }

};

// Forward declaring 
template<>
CObjectManager< CEntity >* getObjectManager<CEntity>();

extern VHandles getEntitiesByString(const std::string& token, std::function<bool(CHandle)> fn = nullptr);
extern VHandles getEntitiesByName(const std::string& name);
extern CHandle getEntityByName(const std::string& name);
#define GameManager static_cast<CEntity*>(getEntityByName("game_manager"))
// #define GameManager static_cast<TCompGameManager*>(static_cast<CEntity*>(getEntityByName("game_manager"))->get<TCompGameManager>())

// After the entity is declared, because it uses the CEntity class
#include "entity/msgs.h"