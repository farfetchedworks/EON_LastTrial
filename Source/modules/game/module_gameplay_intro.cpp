#include "mcv_platform.h"
#include "engine.h"
#include "render/draw_primitives.h"
#include "render/render_module.h"
#include "input/input_module.h"
#include "module_gameplay_intro.h"
#include "modules/module_subtitles.h"
#include "ui/ui_module.h"
#include "lua/module_scripting.h"
#include "modules/module_boot.h"
#include "modules/module_camera_mixer.h"
#include "components/common/comp_camera.h"
#include "components/cameras/comp_camera_flyover.h"
#include "utils/resource_json.h"

extern CShaderCte<CtesWorld> cte_world;

bool ModuleEONGameplayIntro::start()
{
	timer = 1.15f;
	return true;
}

bool ModuleEONGameplayIntro::customStart()
{
	// Tells the boot to load first the intro scenes
	Boot.setIntroBoot(true);
	Boot.customStart();

	Engine.resetClock();

	CEntity* cinematic_cam = getEntityByName("camera_cinematic");

	CModuleCameraMixer& mixer = Engine.getCameramixer();
	mixer.setDefaultCamera(cinematic_cam);
	mixer.setOutputCamera(cinematic_cam);
	mixer.setEnabled(true);
	EngineRender.setActiveCamera(cinematic_cam);

	// Apply tone mapping to frame
	cte_world.in_gameplay = 1.f;

	EngineLua.executeScript("LoreCinematicIntro()");
	Subtitles.startCaption("intro_cinematic");

	// Set initial mouse state
	debugging = false;
	CApplication::get().changeMouseState(debugging, true);

	started = true;

	return true;
}

void ModuleEONGameplayIntro::stop()
{
	Boot.reset();
	cte_world.in_gameplay = 0.f;
	started = false;

	CModuleCameraMixer& mixer = Engine.getCameramixer();
	mixer.setEnabled(false);

	Subtitles.stopCaption();
}

void ModuleEONGameplayIntro::update(float dt)
{
	if (!started)
	{
		if (timer < 0.f)
		{
			customStart();
		}

		// Skip
		timer -= dt;
		return;
	}
		
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
	}

	if (Engine.getInput(input::MENU)->getButton("interact").getsPressed()) {

		EngineLua.executeScript("stopCinematic(1.0)");
		CEngine::get().getModuleManager().changeToGamestate("loading");
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