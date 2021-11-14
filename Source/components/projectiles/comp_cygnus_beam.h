#pragma once

#include "components/common/comp_base.h"
#include "entity/entity.h"
#include "components/messages.h"

class TCompCygnusBeam : public TCompBase {

	DECL_SIBLING_ACCESS();

private:

	int damage = 10;

	CHandle h_collider;
	CHandle h_transform;

	CTransform		local_trans;

	VEC3 offset;

	void destroy();

	// Called when the projectile has hit anything	
	void onTriggerEnter(const TMsgEntityTriggerEnter& msg);

public:

	static void registerMsgs() {
		DECL_MSG(TCompCygnusBeam, TMsgEntityTriggerEnter, onTriggerEnter);
	}

	void setParameters(int damage);

	void load(const json& j, TEntityParseContext& ctx);
	void onEntityCreated();
	void update(float dt);
	void debugInMenu();
};
