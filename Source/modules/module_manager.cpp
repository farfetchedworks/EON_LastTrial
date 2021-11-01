#include "mcv_platform.h"
#include "modules/module_manager.h"

void CModuleManager::boot()
{
  PROFILE_FUNCTION("CModuleManager::boot");

    parseModulesConfig("data/modules.json");
    parseGamestatesConfig("data/gamestates.json");

    if (!_bootGamestate.empty())
    {
        changeToGamestate(_bootGamestate);
    }
}

void CModuleManager::clear()
{
    changeToGamestate(nullptr);

    stopModules(_services);

    _services.clear();
}

void CModuleManager::registerServiceModule(IModule* module)
{
    _registeredModules[module->getName()] = module;

    const bool ok = module->doStart();
    if (ok)
    {
        _services.push_back(module);
    }
}

void CModuleManager::registerGameModule(IModule* module)
{
    _registeredModules[module->getName()] = module;
}

void CModuleManager::registerGamestate(const std::string& gsName, const VModules& modules)
{
    _registeredGamestates[gsName] = modules;
}

void CModuleManager::changeToGamestate(const std::string& gsName)
{
    auto gsIt = _registeredGamestates.find(gsName);

    assert(gsIt != _registeredGamestates.cend());
    if (gsIt == _registeredGamestates.cend()) return;

    changeToGamestate(&(gsIt->second));
}

const CGamestate* CModuleManager::getGamestate(const std::string& gsName)
{
    auto gsIt = _registeredGamestates.find(gsName);

    assert(gsIt != _registeredGamestates.cend());
    if (gsIt == _registeredGamestates.cend()) return nullptr;

    return &(gsIt->second);
}

void CModuleManager::changeToGamestate(CGamestate* gamestate)
{
    _requestedGamestate = gamestate;
}

void CModuleManager::changeToRequestedGamestate()
{
    if (_requestedGamestate == nullptr || _currentGamestate == _requestedGamestate)
    {
        return;
    }

    if (_currentGamestate)
    {
        stopModules(*_currentGamestate);
        _currentGamestate = nullptr;
    }

    if (_requestedGamestate)
    {
        startModules(*_requestedGamestate);
        _currentGamestate = _requestedGamestate;
        _requestedGamestate = nullptr;
    }
}

void CModuleManager::update(float dt)
{
    PROFILE_FUNCTION("CModuleManager::update");
    changeToRequestedGamestate();

    for (auto module : _updatedModules)
    {
        if (module->isActive())
        {
            PROFILE_FUNCTION(module->getName().c_str());
            module->update(dt);
        }
    }
}

void CModuleManager::render()
{
  PROFILE_FUNCTION("CModuleManager::render");
  for (auto module : _renderedModules)
  {
    if (module->isActive())
    {
      module->render();
    }
  }
}

void CModuleManager::renderUI()
{
  PROFILE_FUNCTION("CModuleManager::renderUI");
  for (auto module : _renderedModules)
  {
    if (module->isActive())
    {
      module->renderUI();
    }
  }
}

void CModuleManager::renderDebug()
{
  PROFILE_FUNCTION("CModuleManager::renderDebug");
  CGpuScope gpu_scope("Modules.renderDebug");
  for (auto module : _registeredModules)
  {
    CGpuScope gpu_scope(module.first.c_str());
    module.second->renderDebug();
  }
}

void CModuleManager::renderUIDebug()
{
  PROFILE_FUNCTION("CModuleManager::renderUIDebug");
  CGpuScope gpu_scope("Modules.renderUIDebug");
  for (auto module : _registeredModules)
  {
      if (module.second->isActive())
      {
          CGpuScope gpu_scope(module.first.c_str());
          module.second->renderUIDebug();
      }
  }
}

void CModuleManager::startModules(VModules& modules)
{
    for (auto module : modules)
    {
        module->doStart();
    }
}

void CModuleManager::stopModules(VModules& modules)
{
    for (auto module : modules)
    {
        module->doStop();
    }
}

void CModuleManager::renderInMenu()
{
  for (auto module : _registeredModules)
  {
    module.second->renderInMenu();
  }
}

IModule* CModuleManager::getModule(const std::string& name)
{
    auto moduleIt = _registeredModules.find(name);
    return moduleIt != _registeredModules.end() ? moduleIt->second : nullptr;
}

void CModuleManager::parseModulesConfig(const std::string& filename)
{
    _updatedModules.clear();
    _renderedModules.clear();

    json jData = loadJson(filename);

    json jUpdateList = jData["update"];
    json jRenderList = jData["render"];

    for (auto jModule : jUpdateList)
    {
        const std::string& moduleName = jModule.get<std::string>();
        IModule* module = getModule(moduleName);
        assert(module != nullptr);
        if (module)
        {
            _updatedModules.push_back(module);
        }
    }
    for (auto jModule : jRenderList)
    {
        const std::string& moduleName = jModule.get<std::string>();
        IModule* module = getModule(moduleName);
        assert(module != nullptr);
        if (module)
        {
            _renderedModules.push_back(module);
        }
    }
}

void CModuleManager::parseGamestatesConfig(const std::string& filename)
{
    _registeredGamestates.clear();

    json jData = loadJson(filename);

    _bootGamestate = jData["boot"].get<std::string>();

    json jGamestateList = jData["gamestates"];

    for (auto jGamestate : jGamestateList.items())
    {
        const std::string& gsName = jGamestate.key();

        CGamestate gs;
        for (auto jModule : jGamestate.value())
        {
            const std::string& moduleName = jModule.get<std::string>();
            IModule* module = getModule(moduleName);
            gs.push_back(module);
        }

        registerGamestate(gsName, gs);
    }
}

void CModuleManager::onFileChanged(const std::string& filename)
{
  for (auto it : _registeredModules) {
    IModule* module = it.second;
    module->onFileChanged(filename);
  }
}
