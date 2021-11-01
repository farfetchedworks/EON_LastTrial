#pragma once
#include "handle/handle.h"
#include "entity/entity.h"
#include "components/common/comp_base.h"
#include "components/messages.h"

class TCompNavMesh : public TCompBase {

	DECL_SIBLING_ACCESS();

	CNavMesh* _navMesh	= nullptr;
	bool visible		= true;

public:

	static void registerMsgs() {
		DECL_MSG(TCompNavMesh, TMsgAllEntitiesCreated, onAllEntitiesCreated);
	}

	void onEntityCreated();
	void onAllEntitiesCreated(const TMsgAllEntitiesCreated& msg);
	void load(const json& j, TEntityParseContext& ctx);
	void debugInMenu();
};
