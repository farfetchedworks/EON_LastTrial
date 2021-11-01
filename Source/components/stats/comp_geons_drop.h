#pragma once

#include "entity/entity.h"
#include "components/common/comp_base.h"
#include "components/messages.h"

class TCompGeonsDrop: public TCompBase {

	DECL_SIBLING_ACCESS();

private:
	int geons_dropped	= 0;
	bool doubled		= false;

public:

	void setDoubleDrop() { doubled = true; };
	int getGeonsDropped() { return geons_dropped * (doubled ? 2 : 1); }

	void load(const json& j, TEntityParseContext& ctx);
	void debugInMenu();
};
