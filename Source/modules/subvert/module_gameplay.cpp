#include "mcv_platform.h"
#include "engine.h"
#include "module_gameplay.h"
#include "render/draw_primitives.h"
#include "input/input_module.h"
#include "render/render_module.h"
#include "entity/entity.h"
#include "modules/module_entities.h"
#include "modules/module_boot.h"
#include "modules/module_camera_mixer.h"
#include "modules/module_events.h"
#include "navmesh/module_navmesh.h"
#include "lua/module_scripting.h"
#include "components/messages.h"
#include "components/common/comp_name.h"
#include "components/common/comp_transform.h"
#include "components/common/comp_camera.h"
#include "components/cameras/comp_camera_flyover.h"
#include "components/common/comp_collider.h"
#include "components/common/comp_tags.h"
#include "components/render/comp_irradiance_cache.h"
#include "components/gameplay/comp_game_manager.h"
#include "ui/ui_module.h"
#include "ui/ui_widget.h"

extern CShaderCte<CtesWorld> cte_world;
bool debugging = false;

bool ModuleSubvertGameplay::start()
{
	// set initial mouse state
	debugging = false;
	CApplication::get().changeMouseState(debugging, false);

	CModuleCameraMixer& mixer = CEngine::get().getCameramixer();
	mixer.setOutputCamera(getEntityByName("camera_mixed"));
	EngineRender.setActiveCamera(getEntityByName("camera_mixed"));

	// Apply tone mapping to frame
	cte_world.in_gameplay = 1.f;

	// Activate UI
	EngineUI.activateWidget("subvert_hud");

	// Audio
	EngineLua.executeFile("data/scripts/audio/enterCave.lua");

	// If passing through YOU DIED module
	if (started)
	{
		assert(GameManager);
		static_cast<TCompGameManager*>(GameManager->get<TCompGameManager>())->restartLevel();
		return true;
	}

	// Enable 3d preview mode (for artists)
	if (getEntityByName("camera_preview").isValid())
	{
		mixer.blendCamera("camera_preview", 0.f);
	}
	else
	{
		CEntity* e_player = getEntityByName("player");
		if (e_player) {
			mixer.setDefaultCamera(getEntityByName("camera_follow"));
			mixer.blendCamera("camera_follow", 0.f);
		}
		else {
			CEntity* e_camera_flyover = getEntityByName("camera_flyover");
			TCompCameraFlyover* c_camera_flyover = e_camera_flyover->get<TCompCameraFlyover>();
			c_camera_flyover->enable();
			mixer.setDefaultCamera(e_camera_flyover);
			mixer.blendCamera("camera_flyover", 0.f);
			debugging = true;
			CApplication::get().changeMouseState(debugging);
		}

		// Spawn Eon in the player start position
		VHandles v_player_start = CTagsManager::get().getAllEntitiesByTag(getID("player_start"));

		if (e_player) {
			if (v_player_start.empty()) {
				e_player->setPosition(VEC3::Zero, true);
			}
			else {
				CEntity* e_player_start = v_player_start[0];
				TCompTransform* h_start_trans = e_player_start->get<TCompTransform>();
				e_player->setTransform(*h_start_trans, true);
			}
		}
	}

	started = true;

	return true;
}

void ModuleSubvertGameplay::stop()
{
	cte_world.in_gameplay = 0.f;
}

void ModuleSubvertGameplay::update(float dt)
{
	// DEBUG SHORTCUTS
	{
		// get/release mouse
		if (PlayerInput[VK_F1].getsReleased()) {
			debugging = !debugging;
			CApplication::get().changeMouseState(debugging);
		}
		if (debugging && PlayerInput[VK_CONTROL].getsPressed()) {
			CApplication::get().changeMouseState(false);
		}
		if (debugging && PlayerInput[VK_CONTROL].getsReleased()) {
			CApplication::get().changeMouseState(true);
		}

		// debug Navigation mesh
		if (PlayerInput['N'].getsPressed()) {
			EngineNavMesh.toggle();
		}

		// debug irradiance Probes
		if (PlayerInput['P'].getsPressed()) {
			TCompIrradianceCache::GLOBAL_RENDER_PROBES = !TCompIrradianceCache::GLOBAL_RENDER_PROBES;
		}

		// Capture frames for profiling
		if (PlayerInput['C'].getsPressed()) {
			PROFILE_SET_NFRAMES(3);
			dbg("3 FRAMES CAPTURED\n");
		}

		// open Lua console
		if (PlayerInput['L'].getsPressed()) {
			EngineLua.openConsole();
		}

		// render Imgui in any mode
		if (PlayerInput['I'].getsPressed()) {
			CRenderModule::RENDER_IMGUI = !CRenderModule::RENDER_IMGUI;
		}
	}

	CEntity* e_camera = EngineRender.getActiveCamera();
	if (!e_camera)
	{
		e_camera = getEntityByName("camera_follow");
	}

	if (e_camera)
	{
		assert(e_camera);
		TCompCamera* c_camera = e_camera->get<TCompCamera>();
		assert(c_camera);
		c_camera->setAspectRatio((float)Render.getWidth() / (float)Render.getHeight());
		activateCamera(*c_camera);
	}

	activateObject(MAT44::Identity);
}