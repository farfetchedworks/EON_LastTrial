#pragma once
#include "modules/module.h"

class CModuleEventSystem : public IModule {

	static unsigned int getNextCallbackUniqueID() {
		static unsigned int unique_cb_id = 0;
		++unique_cb_id;
		return unique_cb_id;
	}

	struct TEventCallback {
		unsigned int id = getNextCallbackUniqueID();
		CHandle observer;
		std::function<void(CHandle, CHandle)> fn;
	};

	struct TCallbackInfo {
		unsigned int id;
		std::string name;
	};

	// TODO: mirar el multimap
	std::unordered_map<std::string, std::vector<TEventCallback>> event_callbacks;
	std::vector<TCallbackInfo> unregister_pending;

	bool atUnregisterPendingList(unsigned int id);

public:
	void unregisterPending();
	CModuleEventSystem(const std::string& name) : IModule(name) { }

	bool start() override;
	void stop() override;
	void renderInMenu() override;

	void registerGlobalEvents();

	unsigned int registerEventCallback(const std::string& name, CHandle observer, std::function<void(CHandle, CHandle)> callback);
	unsigned int registerEventCallback(const std::string& name, std::function<void(CHandle, CHandle)> callback);
	void unregisterEventCallback(const std::string& name, unsigned int id);
	void dispatchEvent(const std::string& name, CHandle trigger = CHandle());
};
