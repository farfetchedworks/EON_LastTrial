#pragma once
#include "comp_base.h"
#include "components/messages.h"

struct TCompInstancing : public TCompBase {

	void load(const json& j, TEntityParseContext& ctx);
	void onEntityCreated();
	void onAllEntitiesCreated(const TMsgAllEntitiesCreated& msg);

	static void registerMsgs() {
		DECL_MSG(TCompInstancing, TMsgAllEntitiesCreated, onAllEntitiesCreated);
	}

private:
	void setInstancedPrefab();
	CHandle getPrefab();

	std::string prefab_name;
	static std::map<std::string, CHandle> prefabs;
	static VHandles instanced_prefabs;
};