#include "mcv_platform.h"
#include "engine.h"
#include "module_player_interaction.h"
#include "modules/module_physics.h"
#include "render/render_module.h"
#include "input/input_module.h"
#include "components/common/comp_name.h"
#include "components/common/comp_transform.h"
#include "components/common/comp_camera.h"
#include "components/gameplay/comp_shrine.h"
#include "components/gameplay/comp_launch_animation.h"
#include "components/gameplay/comp_energy_wall.h"
#include "components/controllers/comp_player_controller.h"
#include "components/messages.h"
#include "ui/ui_module.h"
#include "ui/ui_widget.h"

bool CModulePlayerInteraction::start()
{
	h_player = getEntityByName("player");
	return true;
}

void CModulePlayerInteraction::enableUI(bool v)
{
	// Don't activate/deactivate each frame..
	if (v != lastWidgetActive) {

		EngineUI.setWidgetActive("eon_interact", v);
		lastWidgetActive = v;
	}
}

void CModulePlayerInteraction::checkInteractions()
{
	CEntity* player = h_player;
	if (!h_player.isValid())
	{
		player = h_player = getEntityByName("player");
		return;
	}

	TCompTransform* c_transform = player->get<TCompTransform>();
	VEC3 overlap_pos = c_transform->getPosition() + c_transform->getForward();

	VHandles objects_in_area;
	bool is_ok = EnginePhysics.overlapSphere(overlap_pos, 1.f, objects_in_area, CModulePhysics::FilterGroup::Interactable);
	if (is_ok) {
		// Animation launcher can only be executed if at certain distance
		TCompCollider* c_collider = objects_in_area[0];
		CEntity* e_interactable = c_collider->getEntity();
		TCompLaunchAnimation* c_animation = e_interactable->get<TCompLaunchAnimation>();
		TCompShrine* c_shrine = e_interactable->get<TCompShrine>();
		if (c_animation)
			is_ok &= c_animation->resolve();
		else if(c_shrine)
			is_ok &= c_shrine->resolve();

		if (is_ok) {
			auto w = EngineUI.getWidget("eon_interact");

			CEntity* e_camera = EngineRender.getActiveCamera();
			TCompCamera* cam = e_camera->get<TCompCamera>();

			// Set 3D position: interactable pos at player's height + 1.5
			VEC3 posUI = e_interactable->getPosition();
			posUI.y = c_transform->getPosition().y + 1.5f;
			w->setPosition(cam->project(posUI, EngineUI.getResolution()));

			if (PlayerInput["interact"].getsPressed()) {
				interact(c_collider);
			}
		}
	}

	if (active)
		return;

	enableUI(is_ok);
}

void CModulePlayerInteraction::interact(CHandle object)
{
	CEntity* player = h_player;
	TCompPlayerController* controller = player->get<TCompPlayerController>();

	// Currently it can interact with only one interactable. If there is more than one, they must be stacked and the player must be able to choose with which one it interacts
	// Send a message to the interactable
	TCompCollider* c_collider = object;
	CEntity* e_interactable = c_collider->getEntity();
	e_interactable->sendMsg(TMsgEonInteracted({ player }));

	// Remove lock on when interacting with something
	controller->removeLockOn();

	TCompShrine* c_shrine = e_interactable->get<TCompShrine>();
	TCompEnergyWall* c_energyWall = e_interactable->get<TCompEnergyWall>();
	TCompLaunchAnimation* c_animation = e_interactable->get<TCompLaunchAnimation>();

	if (c_shrine) {
		setLastShrine(object.getOwner());
		controller->setVariable("is_praying", true);
	}
	else if (c_energyWall) {
		h_currEnergyWall = c_energyWall;
	}
	else if (c_animation) {
		c_animation->launch();
	}
	else {
		controller->setVariable("is_interacting", true);
	}

	PlayerInteraction.setActive(true);
	enableUI(false);
}

bool CModulePlayerInteraction::isEnergyWallActive()
{
	return h_currEnergyWall.isValid() && static_cast<TCompEnergyWall*>(h_currEnergyWall)->isActive();
}