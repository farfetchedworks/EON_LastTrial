#pragma once

#include "modules/module.h"
#include "entity/entity.h"

class CModuleEntities : public IModule
{
  std::vector< CHandleManager* > om_to_update;
  std::vector< CHandleManager* > om_to_render_debug;

  void loadListOfManagers(const json& j, std::vector< CHandleManager* >& managers);
  void renderDebugOfComponents();
  void editRenderDebug();

  char buff[128];

public:
  CModuleEntities(const std::string& name) : IModule(name) { }
  bool start() override;
  void stop() override;
  void render() override;
  void update(float delta) override;
  void renderDebug() override;
  void renderInMenu() override;

  void clearDebugList();
  void enableBasicDebug();
};

bool isAlive(const std::string& name);
CHandle getEntityByName(const std::string& name);
VHandles getEntitiesByString(const std::string& token, std::function<bool(CHandle)> fn);
VHandles getEntitiesByName(const std::string& name);