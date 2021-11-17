#pragma once

#include "modules/module_manager.h"
#include "input/input.fwd.h"

class CRenderModule;
class CModuleEntities;
class CModulePhysics;
class CModuleCameraMixer;
class CModuleNavMesh;
class CModuleEventSystem;
class CModuleTools;
class CModuleScripting;
class CModuleAudio;
class CModuleBoot;
class CModuleSubtitles;
class CGPUCullingModule;
class CModuleFluidSimulation;
class CModuleIrradianceCache;
class CModuleParticlesEditor;
class CModuleMultithreading;
class CModulePlayerInteraction;

namespace input { class CModule; }
namespace ui { class CModule; }

class CEngine
{

public:
  static CEngine& get();
  std::vector<input::CModule*> getInputs() { return _input; };
  std::thread::id getMainThreadId();

  void init();
  void destroy();
  void doFrame();

  input::CModule* getInput(int id = input::PLAYER_1);
  ui::CModule& getUI() { return *_ui; }
  CModuleManager& getModuleManager() { return _moduleManager; }
  CModuleBoot& getBoot() { return *_boot; };
  CRenderModule& getRender();
  CModuleEntities& getEntities() { return *_entities; }
  CModulePhysics& getPhysics() { return *_physics; }
  CModuleCameraMixer& getCameramixer() { return *_cameraMixer; }
  CModuleNavMesh& getNavMesh() { return *_navMesh; }
  CModuleEventSystem& getEventSystem() { return *_eventSystem; }
  CModuleTools& getTools() { return *_tools; }
  CModuleScripting& getScripting() { return *_scripting; }
  CModuleAudio& getAudio() { return *_audio; }
  CGPUCullingModule& getGPUCulling() { return *_gpu_culling; }
  CGPUCullingModule& getGPUCullingShadows() { return *_gpu_culling_shadows; }
  CModuleFluidSimulation& getFluidSimulation() { return *_fluid_simulation; }
  CModuleParticlesEditor& getParticlesEditor() { return *_particlesEditor; }
  CModuleMultithreading& getMultithreading() { return *_multithreading; }
  CModulePlayerInteraction& getPlayerInteraction() { return *_playerInteraction; }
  CModuleSubtitles& getSubtitles() { return *_subtitles; }

private:
  void update(float dt);
  void render();
  void registerResourceTypes();
  void initInput(input::CModule* input);

  CTimer         timer;
  CModuleManager _moduleManager;

  CModuleBoot* _boot = nullptr;
  CRenderModule* _render = nullptr;
  CModuleEntities* _entities = nullptr;
  CModulePhysics* _physics = nullptr;
  CModuleCameraMixer* _cameraMixer = nullptr;
  CModuleNavMesh* _navMesh = nullptr;
  CModuleEventSystem* _eventSystem = nullptr;
  CModuleTools* _tools = nullptr;
  CModuleScripting* _scripting = nullptr;
  CModuleAudio* _audio = nullptr;
  CGPUCullingModule* _gpu_culling = nullptr;
  CGPUCullingModule* _gpu_culling_shadows = nullptr;
  CModuleFluidSimulation* _fluid_simulation = nullptr;
  CModuleParticlesEditor* _particlesEditor = nullptr;
  CModuleMultithreading* _multithreading = nullptr;
  CModulePlayerInteraction* _playerInteraction = nullptr;
  CModuleSubtitles* _subtitles = nullptr;

  std::thread::id _mainThreadId;
  std::vector<input::CModule*> _input;
  ui::CModule* _ui = nullptr;
};

extern bool debugging;

#define CameraMixer CEngine::get().getCameramixer()
#define PlayerInput CEngine::get().getInput()[input::PLAYER_1]
#define EventSystem CEngine::get().getEventSystem()
#define EnginePhysics CEngine::get().getPhysics()
#define EngineRender CEngine::get().getRender()
#define EngineNavMesh CEngine::get().getNavMesh()
#define EngineTools CEngine::get().getTools()
#define EngineLua CEngine::get().getScripting()
#define EngineUI CEngine::get().getUI()
#define Subtitles CEngine::get().getSubtitles()
#define EngineAudio CEngine::get().getAudio()
#define EngineCulling CEngine::get().getGPUCulling()
#define EngineCullingShadows CEngine::get().getGPUCullingShadows()
#define EngineFluidSimulation CEngine::get().getFluidSimulation()
#define Entities CEngine::get().getEntities()
#define ParticlesEditor CEngine::get().getParticlesEditor()
#define Multithreading CEngine::get().getMultithreading()
#define PlayerInteraction CEngine::get().getPlayerInteraction()
#define Boot CEngine::get().getBoot()

#define Engine CEngine::get()