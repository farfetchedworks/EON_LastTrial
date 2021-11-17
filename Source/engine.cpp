#include "mcv_platform.h"
#include "engine.h"
#include "render/textures/material.h"
#include "geometry/curve.h"
#include "fsm/fsm.h"
#include "fsm/fsm_parser.h"
#include "animation/rigid/rigid_animation_data.h"
#include "particles/particle_emitter.h"

#include "bt/bt.h"
#include "bt/bt_parser.h"

#include "render/render_module.h"
#include "render/gpu_culling_module.h"
#include "modules/module_entities.h"
#include "modules/module_physics.h"
#include "modules/module_camera_mixer.h"
#include "modules/module_boot.h"
#include "modules/module_events.h"
#include "modules/module_particles_editor.h"
#include "modules/module_tools.h"
#include "modules/module_multithreading.h"
#include "modules/module_subtitles.h"
#include "audio/module_audio.h"
#include "lua/module_scripting.h"
#include "navmesh/module_navmesh.h"
#include "input/input_module.h"
#include "input/devices/device_keyboard_windows.h"
#include "input/devices/device_mouse_windows.h"
#include "input/devices/device_pad_xbox_windows.h"
#include "ui/ui_module.h"
#include "render/textures/video/video_texture.h"

// Eon's modules
#include "modules/game/module_splash_screen.h"
#include "modules/game/module_loading_screen.h"
#include "modules/game/module_main_menu.h"
#include "modules/game/module_gameplay.h"
#include "modules/game/module_you_died.h"
#include "modules/game/module_end_game.h"
#include "modules/game/module_fluid_simulation.h"
#include "modules/game/module_irradiance_cache.h"
#include "modules/game/module_player_interaction.h"

#include "utils/directory_watcher.h"
#include "utils/resource_json.h"
#include "skeleton/game_core_skeleton.h"

CDirectoyWatcher dir_watcher_data;
class CComputeShader;

CEngine& CEngine::get()
{
  static CEngine instance;
  return instance;
}

void CEngine::registerResourceTypes()
{
  Resources.registerResourceType(getClassResourceType<CMesh>());
  Resources.registerResourceType(getClassResourceType<CTexture>());
  Resources.registerResourceType(getClassResourceType<CPipelineState>());
  Resources.registerResourceType(getClassResourceType<CMaterial>());
  Resources.registerResourceType(getClassResourceType<CCurve>());
  Resources.registerResourceType(getClassResourceType<CJson>());
  Resources.registerResourceType(getClassResourceType<CGameCoreSkeleton>());
  Resources.registerResourceType(getClassResourceType<fsm::CFSM>());
  Resources.registerResourceType(getClassResourceType<particles::CEmitter>());
  Resources.registerResourceType(getClassResourceType<CBTree>());
  Resources.registerResourceType(getClassResourceType<CNavMesh>());
  Resources.registerResourceType(getClassResourceType<CComputeShader>());
  Resources.registerResourceType(getClassResourceType<TCoreAnimationData>());
}

void CEngine::init()
{
  PROFILE_FUNCTION("Engine::init");

  _mainThreadId = std::this_thread::get_id();

  static ModuleEONSplashScreen splash_screen_module("eon_splash_screen");
  static ModuleEONLoadingScreen loading_screen_module("eon_loading_screen");
  static ModuleEONMainMenu mainmenu_module("eon_main_menu");
  static ModuleEONGameplay gameplay_module("eon_gameplay");
  static ModuleEONYouDied you_died_module("eon_you_died");
  static CModuleIrradianceCache irradiance_cache_module("eon_irradiance_cache");
  static ModuleEONEndGame endgame_module("eon_end_game");

  registerResourceTypes();

  bool is_ok = VideoTexture::createAPI();

  input::CModule::loadDefinitions("data/input/definitions.json");
  fsm::CParser::registerTypes();
  CBTParser::registerTypes();

  // services modules
  _boot = new CModuleBoot("boot");
  _render = new CRenderModule("render");
  _entities = new CModuleEntities("entities");
  _physics = new CModulePhysics("physics");
  _cameraMixer = new CModuleCameraMixer("camera_mixer");
  _input.push_back(new input::CModule("input_1", input::PLAYER_1));
  _input.push_back(new input::CModule("input_2", input::MENU));
  _navMesh = new CModuleNavMesh("navmesh");
  _eventSystem = new CModuleEventSystem("event_system");
  _tools = new CModuleTools("tools");
  _scripting = new CModuleScripting("scripting");
  _ui = new ui::CModule("ui");
  _audio = new CModuleAudio("audio");
  _gpu_culling = new CGPUCullingModule("gpu_culling", eRenderChannel::RC_SOLID);
  _gpu_culling_shadows = new CGPUCullingModule("gpu_culling_shadows", eRenderChannel::RC_SHADOW_CASTERS);
  _fluid_simulation = new CModuleFluidSimulation("fluid_simulation");
  _playerInteraction = new CModulePlayerInteraction("player_interaction");
  _particlesEditor = new CModuleParticlesEditor("particles_editor");
  _multithreading = new CModuleMultithreading("multithreading_module");
  _subtitles = new CModuleSubtitles("subtitles");

  _moduleManager.registerServiceModule(_render);
  _moduleManager.registerServiceModule(_entities);
  _moduleManager.registerServiceModule(_physics);
  _moduleManager.registerServiceModule(_cameraMixer);
  _moduleManager.registerServiceModule(_navMesh);
  _moduleManager.registerServiceModule(_eventSystem);
  _moduleManager.registerServiceModule(_tools);
  _moduleManager.registerServiceModule(_scripting);
  _moduleManager.registerServiceModule(_ui);
  _moduleManager.registerServiceModule(_audio);
  _moduleManager.registerServiceModule(_gpu_culling);
  _moduleManager.registerServiceModule(_gpu_culling_shadows);
  _moduleManager.registerServiceModule(_fluid_simulation);
  _moduleManager.registerServiceModule(_particlesEditor);
  _moduleManager.registerServiceModule(_multithreading);
  _moduleManager.registerServiceModule(_subtitles);

  for (auto input : _input)
  {
    _moduleManager.registerServiceModule(input);
    initInput(input);
  }

  // game modules
  _moduleManager.registerGameModule(_boot);

  // Eon's specific
  _moduleManager.registerGameModule(&splash_screen_module);
  _moduleManager.registerGameModule(&loading_screen_module);
  _moduleManager.registerGameModule(&mainmenu_module);
  _moduleManager.registerGameModule(&gameplay_module);
  _moduleManager.registerGameModule(&you_died_module);
  _moduleManager.registerGameModule(_playerInteraction);
  _moduleManager.registerGameModule(&irradiance_cache_module);
  _moduleManager.registerGameModule(&endgame_module);

  // boot
  _moduleManager.boot();

  dir_watcher_data.start("data", CApplication::get().getHandle());

  timer.reset();
}

void CEngine::destroy()
{
  VideoTexture::destroyAPI();
  _moduleManager.clear();
  Resources.destroy();
  delete _render;
}

void CEngine::doFrame()
{
  PROFILE_FRAME_BEGINS();
  PROFILE_FUNCTION("Engine::doFrame");
  Time.set(timer.elapsed());
  update(Time.delta);
  // Let the renderModule generate the frame
  _render->generateFrame();
}

void CEngine::update(float dt)
{
  _moduleManager.update(dt);
}

void CEngine::render()
{
  _moduleManager.render();
}

CRenderModule& CEngine::getRender()
{
  assert(_render != nullptr);
  return *_render;
}

input::CModule* CEngine::getInput(int id)
{
  input::CModule* requiredInput = nullptr;

  for (auto input : _input)
  {
    if (input->getId() == id)
    {
      requiredInput = input;
    }
  }

  assert(requiredInput != nullptr);
  return requiredInput;
}

void CEngine::initInput(input::CModule* input)
{
    assert(input);

    input->registerDevice(new input::CDeviceKeyboardWindows());
    input->registerDevice(new input::CDeviceMouseWindows(CApplication::get().getHandle()));
    input->registerDevice(new input::CDevicePadXboxWindows(0));

    input->loadMapping("data/input/mappings.json");
}

std::thread::id CEngine::getMainThreadId()
{
    return _mainThreadId;
}