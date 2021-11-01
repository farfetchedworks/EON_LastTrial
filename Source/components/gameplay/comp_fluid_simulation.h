#pragma once

#include "components/common/comp_base.h"
#include "entity/entity.h"
#include "components/messages.h"

class TCompFluidSimulation : public TCompBase {

	DECL_SIBLING_ACCESS();

private:


public:

	void onEntityCreated();
	void update(float dt);
	void debugInMenu();
	void renderDebug();
	void load(const json& j, TEntityParseContext& ctx);

};
