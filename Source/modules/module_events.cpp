#include "mcv_platform.h"
#include "engine.h"
#include "module_events.h"
#include "entity/entity_parser.h"
#include "ui/ui_module.h"
#include "components/messages.h"
#include "input/input_module.h"
#include "components/common/comp_parent.h"
#include "modules/game/module_player_interaction.h"
#include "modules/module_camera_mixer.h"
#include "lua/module_scripting.h"
#include "skeleton/comp_attached_to_bone.h"
#include "components/ai/comp_bt.h"
#include "components/common/comp_parent.h"
#include "components/cameras/comp_camera_follow.h"
#include "components/controllers/comp_rigid_animation_controller.h"
#include "components/controllers/comp_focus_controller.h"
#include "components/controllers/comp_player_controller.h"
#include "components/controllers/pawn_utils.h"
#include "components/gameplay/comp_game_manager.h"
#include "components/gameplay/comp_shrine.h"
#include "components/abilities/comp_area_delay.h"
#include "components/abilities/comp_time_reversal.h"
#include "components/postfx/comp_color_grading.h"
#include "entity/entity_parser.h"
#include "components/ai/comp_bt.h"
#include "components/stats/comp_health.h"

extern CShaderCte<CtesWorld> cte_world;

bool CModuleEventSystem::start()
{
	event_callbacks = {};
	unregister_pending = {};

	registerGlobalEvents();

	return true;
}

void CModuleEventSystem::stop()
{

}

unsigned int CModuleEventSystem::registerEventCallback(const std::string& name, CHandle observer, std::function<void(CHandle, CHandle)> callback)
{
	TEventCallback data;

	data.observer = observer;
	data.fn = callback;

	event_callbacks[name].push_back(data);
	return data.id;
}

unsigned int CModuleEventSystem::registerEventCallback(const std::string& name, std::function<void(CHandle, CHandle)> callback)
{
	TEventCallback data;

	data.observer = CHandle();
	data.fn = callback;

	event_callbacks[name].push_back(data);
	return data.id;
}

void CModuleEventSystem::dispatchEvent(const std::string& name, const std::string& trigger_name)
{
	CEntity* trigger = getEntityByName(trigger_name);
	dispatchEvent(name, trigger);
}

void CModuleEventSystem::dispatchEvent(const std::string& name, CHandle trigger)
{
	if (event_callbacks.find(name) == event_callbacks.end()) {
		dbg("Event %s not found!\n", name.c_str());
		return;
	}

	std::vector<TEventCallback>& callbacks = event_callbacks[name];

	for (auto& cb : callbacks)
	{
		if (atUnregisterPendingList(cb.id))
			continue;

		cb.fn(trigger, cb.observer);
	}
}

void CModuleEventSystem::unregisterEventCallback(const std::string& name, unsigned int id)
{
	assert(event_callbacks.find(name) != event_callbacks.end());

	auto it = std::find_if(begin(event_callbacks[name]), end(event_callbacks[name]), [id](const TEventCallback& cb) {
		return cb.id == id;
	});

	assert(it != event_callbacks[name].end());
	unregister_pending.push_back({ id, name });

	unregisterPending();
}

void CModuleEventSystem::unregisterPending()
{
	for (auto& cbInfo : unregister_pending) {

		const unsigned int id = cbInfo.id;
		const std::string& name = cbInfo.name;
		std::vector<TEventCallback>& callbacks = event_callbacks[name];

		auto it = std::remove_if(begin(callbacks), end(callbacks), [id](const TEventCallback& cb) {return cb.id == id; });
		if(it != callbacks.end())
			callbacks.erase(it, end(callbacks));

		if (!callbacks.size())
		{
			event_callbacks.erase(name);
		}
	}

	unregister_pending.clear();
}

bool CModuleEventSystem::atUnregisterPendingList(unsigned int id)
{
	auto it = std::find_if(begin(unregister_pending), end(unregister_pending), [id](const TCallbackInfo& cbi) {return cbi.id == id; });
	return it != unregister_pending.end();
}

void CModuleEventSystem::registerGlobalEvents()
{
	// Some global events

	EventSystem.registerEventCallback("Input/block", [](CHandle t, CHandle o) {
		PlayerInput.blockInput();
	});

	EventSystem.registerEventCallback("Input/unblock", [](CHandle t, CHandle o) {
		PlayerInput.unBlockInput();
	});

	EventSystem.registerEventCallback("Render/lutIn", [](CHandle t, CHandle o)  {
		CEntity* cam = getEntityByName("camera_mixed");
		TCompColorGrading* lut = cam->get<TCompColorGrading>();
		lut->fadeIn(2.f, &interpolators::cubicOutInterpolator);
	});

	EventSystem.registerEventCallback("Render/lutOut", [](CHandle t, CHandle o) {
		CEntity* cam = getEntityByName("camera_mixed");
		TCompColorGrading* lut = cam->get<TCompColorGrading>();
		lut->fadeOut(5.5f, &interpolators::quadInInterpolator);
	});

	EventSystem.registerEventCallback("Gameplay/Eon/setLocomotion", [](CHandle t, CHandle o) {

		CEntity* e = getEntityByName("player");
		assert(e);
		if (!e)
			return;
		TCompPlayerController* controller = e->get<TCompPlayerController>();
		if(controller->getSpeed() > 0.1f)
			PawnUtils::stopAction(e, "AreaDelay_Throw", 0.2f);
	});

	EventSystem.registerEventCallback("Gameplay/Eon/detachAD", [](CHandle t, CHandle o) {

		CEntity* e = getEntityByName("player");
		assert(e);
		if (!e)
			return;
		TCompAreaDelay* c_ad = e->get<TCompAreaDelay>();
		c_ad->detachADBall();
		e->sendMsg(TMsgStopAiming({ 0.5f }));
		});

	EventSystem.registerEventCallback("Gameplay/Animation/detachSocket", [](CHandle t, CHandle o) {
		CEntity* e = t;
		assert(e);
		if (!e)
			return;
		TCompParent* parent = e->get<TCompParent>();
		for (auto& h : parent->children)
		{
			CEntity* child = h;
			TCompAttachedToBone* socket = child->get<TCompAttachedToBone>();
			if (!socket)
				continue;
			socket->detach();
			TCompRigidAnimationController * controller = child->get<TCompRigidAnimationController>();
			if (controller)
				controller->start();
		}
	});

	EventSystem.registerEventCallback("Gameplay/Animation/attachSocket", [](CHandle t, CHandle o) {
		CEntity* e = t;
		assert(e);
		if (!e)
			return;
		TCompParent* parent = e->get<TCompParent>();
		for (auto& h : parent->children)
		{
			CEntity* child = h;
			TCompAttachedToBone* socket = child->get<TCompAttachedToBone>();
			if (!socket)
				continue;
			socket->attach();
			TCompRigidAnimationController* controller = child->get<TCompRigidAnimationController>();
			if (controller)
				controller->stop();
		}
	});

	EventSystem.registerEventCallback("Gameplay/Eon/onShrineActivated", [](CHandle t, CHandle o) {
		TCompGameManager* comp_gm = GameManager->get<TCompGameManager>();
		comp_gm->respawnEnemies();
		CEntity* owner = t;
		assert(owner);
		TCompShrine* shrine = owner->get<TCompShrine>();
		shrine->restorePlayer();
		TCompTimeReversal* c_time_reversal = owner->get<TCompTimeReversal>();
		c_time_reversal->clearBuffer();
	});

	EventSystem.registerEventCallback("Gameplay/Eon/onInteractionEnded", [](CHandle t, CHandle o) {
		PlayerInteraction.setActive(false);
	});

	EventSystem.registerEventCallback("Gameplay/Eon/removeDynamicCamera", [](CHandle t, CHandle o) {
		CEntity* camera_follow = getEntityByName("camera_follow");
		TCompCameraFollow* c_camera_follow = camera_follow->get<TCompCameraFollow>();
		c_camera_follow->enable();
		CameraMixer.blendCamera("camera_follow", 2.f, &interpolators::quadInOutInterpolator);
		PlayerInput.unBlockInput();
		TCompBT::UPDATE_ENABLED = true;
	});

	EventSystem.registerEventCallback("Gameplay/Eon/openCygnusPath", [](CHandle t, CHandle o) {
		CameraMixer.blendCamera("camera_follow", 1.f, &interpolators::quadInOutInterpolator);
		PlayerInput.unBlockInput();
		EngineUI.fadeOut(1.5f);

		CEntity* owner = getEntityByName("player");
		assert(owner);
		TCompTimeReversal* c_time_reversal = owner->get<TCompTimeReversal>();
		c_time_reversal->renderEffect(false);
	});

	EventSystem.registerEventCallback("Gameplay/runParticles", [](CHandle t, CHandle o) {
		CEntity* owner = t;
		assert(owner);
		TCompTransform* c_trans = owner->get<TCompTransform>();
		spawnParticles("data/particles/compute_run_particles.json", c_trans->getPosition() + c_trans->getForward() * 0.6f, c_trans->getPosition());
	});

	EventSystem.registerEventCallback("Gameplay/Cygnus/floorHitParticles", [](CHandle t, CHandle o) {
		CEntity* owner = t;
		assert(owner);
		TCompTransform* c_trans = owner->get<TCompTransform>();
		spawnParticles("data/particles/compute_sword_sparks_particle.json", c_trans->getPosition() + c_trans->getForward() * 0.44f, c_trans->getPosition());
		spawnParticles("data/particles/compute_sword_sparks_particle.json", c_trans->getPosition() + c_trans->getForward() * 0.5f - c_trans->getRight() * 0.24f, c_trans->getPosition());
	});
	
	EventSystem.registerEventCallback("Gameplay/NPC/Flower", [](CHandle t, CHandle o) {
	
		// Remove flower from player hand
		CEntity* player = getEntityByName("player");
		TCompParent* parent = player->get<TCompParent>();
		CEntity* flor = parent->getChildByName("WeaponFlor");
		TCompTransform* transFlower = flor->get<TCompTransform>();

		CTransform trans;
		trans.fromMatrix(*transFlower);
		spawn("data/prefabs/flor_02.json", trans);
		flor->destroy();
		CHandleManager::destroyAllPendingObjects();

		EngineLua.executeScript("BeginEndLoreCinematic()");
	});

	EventSystem.registerEventCallback("Gameplay/Cygnus/Phase_1_to_2", [](CHandle t, CHandle o) {

		// Set player in initial pos every cinematic
		CEntity* playerTargetEntity = getEntityByName("CygnusBeamPosition2");
		TCompTransform* c_trans_target_pos = playerTargetEntity->get<TCompTransform>();
		CEntity* player = getEntityByName("player");
		TCompTransform* c_trans_player = player->get<TCompTransform>();
		player->setPosition(c_trans_target_pos->getPosition(), true);
		c_trans_player->setRotation(c_trans_target_pos->getRotation());
		
		// Place Cygnus in the center
		CEntity* e_cygnus = getEntityByName("Cygnus_Form_1");
		TCompTransform* transform = e_cygnus->get<TCompTransform>();
		CEntity* e_arenacenter = getEntityByName("CygnusArenaCenter");
		TCompTransform* c_trans_arena = e_arenacenter->get<TCompTransform>();
		e_cygnus->setPosition(c_trans_arena->getPosition(), true);
		transform->setRotation(QUAT::Concatenate(c_trans_arena->getRotation(), QUAT::CreateFromAxisAngle(VEC3::Up, deg2rad(180.f))));

		// Spawn the black hole where Cygnus is
		spawn("data/prefabs/black_hole_cygnus.json", *transform);

		// Get Form 1 info
		CTransform transform_form_1;
		transform_form_1.fromMatrix(*transform);
		float yaw = transform->getYawRotationToAimTo(player->getPosition());
		transform_form_1.setRotation(QUAT::Concatenate(transform_form_1.getRotation(), QUAT::CreateFromYawPitchRoll(yaw, 0.f, 0.f)));

		// Spawn new form
		CEntity* e_cygnus_2 = spawn("data/prefabs/cygnus_form_2.json", transform_form_1);

		CEntity* e_camera = getEntityByName("camera_mixed");
		assert(e_camera);
		TCompFocusController* c_focus = e_camera->get<TCompFocusController>();
		c_focus->enable(e_cygnus_2, 6.0f);

		// Destroy form 1 entity
		e_cygnus->destroy();
		CHandleManager::destroyAllPendingObjects();

		// Intro form 2
		EngineLua.executeScript("CinematicCygnusF1ToF2()");
	});

	EventSystem.registerEventCallback("Gameplay/Cygnus/Phase_2_to_3", [](CHandle t, CHandle o) {

		// Set player in initial pos every cinematic
		CEntity* playerTargetEntity = getEntityByName("CygnusBeamPosition2");
		TCompTransform* c_trans_target_pos = playerTargetEntity->get<TCompTransform>();
		CEntity* player = getEntityByName("player");
		TCompTransform* c_trans_player = player->get<TCompTransform>();
		player->setPosition(c_trans_target_pos->getPosition(), true);
		c_trans_player->setRotation(c_trans_target_pos->getRotation());
		// Reset movements
		TCompPlayerController* controller = player->get<TCompPlayerController>();
		controller->reset();

		// Place Cygnus in the center
		CEntity* e_cygnus = getEntityByName("Cygnus_Form_2");
		TCompTransform* transform = e_cygnus->get<TCompTransform>();
		CEntity* e_arenacenter = getEntityByName("CygnusArenaCenter");
		TCompTransform* c_trans_arena = e_arenacenter->get<TCompTransform>();
		e_cygnus->setPosition(c_trans_arena->getPosition(), true);

		float yaw = transform->getYawRotationToAimTo(player->getPosition());
		transform->setRotation(transform->getRotation() * QUAT::CreateFromYawPitchRoll(yaw, 0.f, 0.f));

		CEntity* e_camera = getEntityByName("camera_mixed");
		assert(e_camera);
		TCompFocusController* c_focus = e_camera->get<TCompFocusController>();
		c_focus->enable(e_cygnus, 6.0f);
	});

	EventSystem.registerEventCallback("Gameplay/Cygnus/FinalDeath", [](CHandle t, CHandle o) {

		// Set player in initial pos every cinematic
		CEntity* playerTargetEntity = getEntityByName("CygnusBeamPosition2");
		TCompTransform* c_trans_target_pos = playerTargetEntity->get<TCompTransform>();
		CEntity* player = getEntityByName("player");
		TCompTransform* c_trans_player = player->get<TCompTransform>();
		player->setPosition(c_trans_target_pos->getPosition(), true);
		c_trans_player->setRotation(c_trans_target_pos->getRotation());
		// Reset movements
		TCompPlayerController* controller = player->get<TCompPlayerController>();
		controller->reset();

		// Place Cygnus in the center
		CEntity* e_cygnus = getEntityByName("Cygnus_Form_2");
		TCompTransform* transform = e_cygnus->get<TCompTransform>();
		CEntity* e_arenacenter = getEntityByName("CygnusArenaCenter");
		TCompTransform* c_trans_arena = e_arenacenter->get<TCompTransform>();
		e_cygnus->setPosition(c_trans_arena->getPosition(), true);

		float yaw = transform->getYawRotationToAimTo(player->getPosition());
		transform->setRotation(QUAT::Concatenate(transform->getRotation(), QUAT::CreateFromYawPitchRoll(yaw, 0.f, 0.f)));
	});

	EventSystem.registerEventCallback("Gameplay/Eon/HoloDestroyed", [](CHandle t, CHandle o) {
		CEntity* player = getEntityByName("player");
		TCompTimeReversal* c_time_reversal = player->get<TCompTimeReversal>();
		c_time_reversal->removeInvisibleEffect();
	});
}

void CModuleEventSystem::renderInMenu()
{
	if (ImGui::TreeNode("All Events...")) {

		for (auto& event : event_callbacks)
		{
			std::string name = event.first;

			if (ImGui::TreeNode(name.c_str())) {

				for (auto& cb : event_callbacks[name])
				{
					ImGui::Text("Callback: %d", cb.id);
				}

				ImGui::TreePop();
			}
		}

		ImGui::TreePop();
	}
}