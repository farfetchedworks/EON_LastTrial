#pragma once
#include "components/common/comp_base.h"
#include "entity/entity.h"
#include "components/messages.h"

class TCompCygnusKey : public TCompBase {

	DECL_SIBLING_ACCESS();

	int _order		= 0;
	bool _active	= false;
	float _waitTime	= -1.f;

public:

	static void registerMsgs() {
		DECL_MSG(TCompCygnusKey, TMsgAllEntitiesCreated, onAllEntitiesCreated);
	}

	void load(const json& j, TEntityParseContext& ctx);
	void onAllEntitiesCreated(const TMsgAllEntitiesCreated& msg);
	void update(float dt);
	void debugInMenu();
	bool resolve();
	void onAllKeysOpened();
	void start(float time = 0.f);
	void setActive();
	int getOrder() { return _order; }
};
