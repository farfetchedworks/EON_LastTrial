#include "mcv_platform.h"
#include "engine.h"
#include "render/draw_primitives.h"
#include "render/render_module.h"
#include "input/input_module.h"
#include "module_gameplay_intro.h"
#include "lua/module_scripting.h"
#include "modules/module_boot.h"
#include "modules/module_camera_mixer.h"
#include "components/common/comp_camera.h"
#include "components/cameras/comp_camera_flyover.h"
#include "utils/resource_json.h"

extern CShaderCte<CtesWorld> cte_world;

bool ModuleEONGameplayIntro::start()
{
	timer = 2.15f;
	return true;
}

bool ModuleEONGameplayIntro::customStart()
{
	// Tells the boot to load first the intro scenes
	Boot.setSlowBoot(true);
	Boot.customStart();

	// Set initial mouse state
	debugging = false;
	CApplication::get().changeMouseState(debugging, false);

	CModuleCameraMixer& mixer = CEngine::get().getCameramixer();
	mixer.setOutputCamera(getEntityByName("camera_cinematic"));
	EngineRender.setActiveCamera(getEntityByName("camera_cinematic"));

	// Apply tone mapping to frame
	cte_world.in_gameplay = 1.f;

	EngineLua.executeScript("CinematicEonIntro()");

	started = true;

	return true;
}

void ModuleEONGameplayIntro::stop()
{
	Boot.reset();
	cte_world.in_gameplay = 0.f;

	CModuleCameraMixer& mixer = CEngine::get().getCameramixer();
	mixer.setEnabled(false);
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
		
	if (CEngine::get().getInput(input::MENU)->getButton("interact").getsPressed()) {
		EngineLua.executeScript("stopCinematic(1.0)");
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