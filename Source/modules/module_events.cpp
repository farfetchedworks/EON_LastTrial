#include "mcv_platform.h"
#include "engine.h"
#include "components/messages.h"
#include "module_events.h"
#include "components/abilities/comp_area_delay.h"
#include "skeleton/comp_attached_to_bone.h"
#include "components/common/comp_parent.h"
#include "components/controllers/comp_rigid_animation_controller.h"

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

	// ..
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
			socket->detach();
			TCompRigidAnimationController * controller = child->get<TCompRigidAnimationController>();
			if (!controller)
				continue;
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
			socket->attach();
			TCompRigidAnimationController* controller = child->get<TCompRigidAnimationController>();
			if (!controller)
				continue;
			controller->stop();
		}
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