#pragma once

#include "comp_base.h"
#include "entity/entity.h"

// Does NOT have an 3D hierarchy dependency
struct TCompParent : public TCompBase {
  VHandles  children;
  DECL_SIBLING_ACCESS();
  ~TCompParent();
  void addChild(CHandle h_new_child);
  bool delChild(CHandle h_curr_child);
  void disableChildCollider();
  void forEachChild(std::function<void(CHandle)> fn);
  CHandle getChildByName(const std::string& name);
  void debugInMenu();
};

// If an entity wants to access his parent, use the CHandle::getOwner()

