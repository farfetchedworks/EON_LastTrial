#pragma once
#include "components/common/comp_base.h"
#include "entity/entity.h"
#include "components/messages.h"

class TCompNPC : public TCompBase {

	DECL_SIBLING_ACCESS();

	bool first_interaction = false;
	bool talk_3d = true;

	VEC3 lookat_offset;
	float sight_radius = 2.f;
	std::string unique_caption_scene;
	std::string caption_scene;

public:
	
	void load(const json& j, TEntityParseContext& ctx);
	void update(float dt);

	bool resolve();
	void interact();

	void onStop(const TMsgStopCaption& msg);

	static void registerMsgs() {
		DECL_MSG(TCompNPC, TMsgStopCaption, onStop);
	}
};
