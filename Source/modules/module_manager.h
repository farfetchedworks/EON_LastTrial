#pragma once

#include "modules/module.h"

class CModuleManager
{
  public:
    void boot();
    void clear();

    void update(float dt);
    void render();
    void renderUI();
    void renderInMenu();
    void renderDebug();
    void renderUIDebug();

    void registerServiceModule(IModule* module);
    void registerGameModule(IModule* module);

    void registerGamestate(const std::string& gsName, const VModules& modules);
    void changeToGamestate(const std::string& gsName);
    void changeToGamestate(CGamestate* gamestate);

    IModule* getModule(const std::string& name);
    const CGamestate* getCurrentGamestate() { return _currentGamestate;  };
    const CGamestate* getGamestate(const std::string& gsName);

    void onFileChanged(const std::string& filename);

  private:
    void parseModulesConfig(const std::string& filename);
    void parseGamestatesConfig(const std::string& filename);

    void startModules(VModules& modules);
    void stopModules(VModules& modules);
    void changeToRequestedGamestate();

    VModules _services;
    VModules _updatedModules;
    VModules _renderedModules;
    std::string _bootGamestate;
    CGamestate* _currentGamestate = nullptr;
    CGamestate* _requestedGamestate = nullptr;

    std::map<std::string, IModule*> _registeredModules;
    std::map<std::string, CGamestate> _registeredGamestates;
};