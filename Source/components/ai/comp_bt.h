#pragma once
#include "handle/handle.h"
#include "entity/entity.h"
#include "components/common/comp_base.h"
#include "components/messages.h"
#include "bt/bt_context.h"

class TCompBT : public TCompBase {

	DECL_SIBLING_ACCESS();

private:
	int MAX_REGULAR_DMG = 50;					// If this damage is exceeded, it must perform a strong damage animation

	CBTContext* bt_context;
	CHandle h_collider;

	// Message callbacks
	void onHit(const TMsgHit& msg);
	void onPreHit(const TMsgPreHit& msg);
	void onParry(const TMsgEnemyStunned& msg);
	////////////////////

public:

	~TCompBT();

	void load(const json& j, TEntityParseContext& ctx);
	void onEntityCreated();

	CBTContext* getContext() { return bt_context; }

	void reload();

	void debugInMenu();
	void renderDebug();
	void update(float dt);

	bool isEnabled() { return bt_context->isEnabled(); }
	void setEnabled(bool enabled) { bt_context->setEnabled(enabled); }

	float getMoveSpeed();
	VEC3 getMoveDirection();

	void onEonIsDead(bool is_eon_dead = true);
	
	bool isDying() const { return bt_context->isDying(); }

	static bool UPDATE_ENABLED;

	static void registerMsgs() {
		DECL_MSG(TCompBT, TMsgHit, onHit);
		DECL_MSG(TCompBT, TMsgPreHit, onPreHit);
		DECL_MSG(TCompBT, TMsgEnemyStunned, onParry);
	}
};
