#include "mcv_platform.h"
#include "logic_manager.h"
#include "engine.h"
#include "entity/entity.h"
#include "lua/module_scripting.h"
#include "components/controllers/comp_curve_controller.h"
#include "components/controllers/comp_rigid_animation_controller.h"
#include "components/gameplay/comp_game_manager.h"
#include "components/cameras/comp_camera_follow.h"
#include "modules/module_camera_mixer.h"
#include "modules/module_subtitles.h"
#include "input/input_module.h"
#include "ui/ui_module.h"

namespace LogicManager
{
	void ldbg(sol::variadic_args args)
	{
		std::string result = "";

		for (auto s : args) {

			// Add here types of variables used in LUA

			if (s.is<std::string>())
				result += s;
			else if (s.is<bool>())
				result += s.get<bool>() ? "true" : "false";
			else if (s.is<int>())
				result += std::to_string(s.get<int>());
			else if (s.is<float>())
				result += std::to_string(s.get<float>());
			else if (s.is<VEC3>())
			{
				VEC3 v = s.get<VEC3>();
				result += "(" + std::to_string(v.x) + ", " + std::to_string(v.y)
					+ ", " + std::to_string(v.z) + ")";
			}
			else if (s.is<CHandle>())
			{
				CEntity* e = s.get<CHandle>();
				result += e ? "[h:" + std::string(e->getName()) + "]" : "[h not valid]";
			}
			else
			{
				// Not supported type, this invalidates the script execution so it's good
				result = s;
				return;
			}

			result += " ";
		}

		result += "\n";

		dbg(result.c_str());
		EngineLua.logConsole(result.c_str());
	}

	void activateWidget(const std::string& name)
	{
		EngineUI.activateWidget(name);
	}

	void deactivateWidget(const std::string& name)
	{
		EngineUI.deactivateWidget(name);
	}

	void fade()
	{
		EngineUI.activateWidget("modal_black");
	}

	void unfade()
	{
		EngineUI.deactivateWidget("modal_black");
	}

	void goToGamestate(const std::string& gs_name)
	{
		CModuleManager& modules = CEngine::get().getModuleManager();
		modules.changeToGamestate(gs_name);
	}

	void startCinematic(const std::string& curve_filename, const VEC3& target, float speed, float lerp_time)
	{
		TCompGameManager* c_gm = GameManager->get<TCompGameManager>();
		c_gm->setIsInCinematic(true);

		CEntity* e_camera_follow = getEntityByName("camera_follow");
		TCompCameraFollow* c_camera_follow = e_camera_follow->get<TCompCameraFollow>();
		c_camera_follow->disable();
		
		PlayerInput.blockInput();

		CEntity* e_camera = getEntityByName("camera_cinematic");
		TCompCurveController* controller = e_camera->get<TCompCurveController>();

		assert(!curve_filename.empty());
		controller->setCurve("data/curves/" + curve_filename);
		controller->setTarget(target);
		controller->setSpeed(speed);

		controller->setActive(true);

		CameraMixer.blendCamera("camera_cinematic", lerp_time, &interpolators::cubicInOutInterpolator);
	}

	void startCinematicAnimation(const std::string& animation_filename, const std::string& target_name, float speed, float lerp_time, const std::string& bone_name)
	{
		assert(!animation_filename.empty());
		CEntity* e_target = getEntityByName(target_name);
		assert(e_target);
		if (!e_target)
			return;

		TCompGameManager* c_gm = GameManager->get<TCompGameManager>();
		c_gm->setIsInCinematic(true);

		CEntity* e_camera_follow = getEntityByName("camera_follow");
		TCompCameraFollow* c_camera_follow = e_camera_follow->get<TCompCameraFollow>();
		c_camera_follow->disable();

		CEntity* e_cinematic_camera = getEntityByName("camera_cinematic");
		assert(e_cinematic_camera);
		TCompRigidAnimationController* controller = e_cinematic_camera->get<TCompRigidAnimationController>();
		assert(controller);
		controller->setTarget(target_name);
		controller->setTargetBone(bone_name);
		controller->setStatic(false);
		controller->setSpeed(speed);
		controller->setAnimation("data/animations/" + animation_filename);
		controller->start();

		PlayerInput.blockInput();

		CameraMixer.blendCamera("camera_cinematic", lerp_time, &interpolators::cubicInOutInterpolator);
	}

	void startStaticCinematicAnimation(const std::string& animation_filename, float speed, float lerp_time)
	{
		assert(!animation_filename.empty());

		TCompGameManager* c_gm = GameManager->get<TCompGameManager>();
		c_gm->setIsInCinematic(true);

		CEntity* e_camera_follow = getEntityByName("camera_follow");
		TCompCameraFollow* c_camera_follow = e_camera_follow->get<TCompCameraFollow>();
		c_camera_follow->disable();

		CEntity* e_cinematic_camera = getEntityByName("camera_cinematic");
		assert(e_cinematic_camera);
		TCompRigidAnimationController* controller = e_cinematic_camera->get<TCompRigidAnimationController>();
		assert(controller);
		controller->setTarget("");
		controller->setStatic(true);
		controller->setTargetBone("");
		controller->setSpeed(speed);
		controller->setAnimation("data/animations/" + animation_filename);
		controller->start();

		CameraMixer.blendCamera("camera_cinematic", lerp_time, &interpolators::cubicInOutInterpolator);
	}

	void stopCinematic(float lerp_time)
	{
		TCompGameManager* c_gm = GameManager->get<TCompGameManager>();
		c_gm->setIsInCinematic(false);

		CEntity* e_camera_follow = getEntityByName("camera_follow");
		TCompCameraFollow* c_camera_follow = e_camera_follow->get<TCompCameraFollow>();
		c_camera_follow->enable();


		CEntity* e_camera = getEntityByName("camera_cinematic");
		TCompTransform* c_trans = e_camera->get<TCompTransform>();

		TCompCurveController* c_curve_controller = e_camera->get<TCompCurveController>();
		TCompRigidAnimationController* c_ra_controller = e_camera->get<TCompRigidAnimationController>();

		if (c_curve_controller->active) {
			c_curve_controller->setActive(false);
			PlayerInput.unBlockInput();
		}
		else
		{
			c_ra_controller->stop();
		}

		CameraMixer.blendCamera("camera_follow", lerp_time, &interpolators::cubicInOutInterpolator);
	}

	void setCinematicCurve(const std::string& curve_filename, float curve_speed, float lerp_time)
	{
		CEntity* e_camera = getEntityByName("camera_cinematic");
		TCompCurveController* c_curve_controller = e_camera->get<TCompCurveController>();

		assert(!curve_filename.empty());
		c_curve_controller->setCurveLerped("data/curves/" + curve_filename, curve_speed, lerp_time);
	}

	void setCinematicTarget(const VEC3& target, float lerp_time)
	{
		CEntity* e_camera = getEntityByName("camera_cinematic");
		TCompCurveController* c_curve_controller = e_camera->get<TCompCurveController>();

		if (c_curve_controller) {
			c_curve_controller->setTargetLerped(target, lerp_time);
		}
	}

	void setCinematicTargetEntity(const std::string& entity_name, float lerp_time)
	{
		CEntity* e_camera = getEntityByName("camera_cinematic");
		TCompCurveController* c_curve_controller = e_camera->get<TCompCurveController>();
		c_curve_controller->setTargetEntity(entity_name);
	}

	VEC3 getEntityPosByName(const std::string& entity_name)
	{
		CEntity* entity = getEntityByName(entity_name);
		TCompTransform* c_trans = entity->get<TCompTransform>();

		assert(c_trans);
		if (!c_trans) {
			return {};
		}

		return c_trans->getPosition();
	}

}
