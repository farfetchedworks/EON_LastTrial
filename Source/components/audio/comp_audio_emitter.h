#include "mcv_platform.h"
#include "entity/entity.h"
#include "components/common/comp_base.h"
#include "components/messages.h"
#include "fmod_studio.hpp"

class TCompAudioEmitter : public TCompBase {

	DECL_SIBLING_ACCESS();

private:

	CHandle h_cache_transform;
	CHandle h_cache_player_transform;

	std::string event_name;
	std::string bank_name;
	FMOD::Studio::EventInstance* event_inst;

	bool static_inst = false;
	bool updates_occl = true;

	unsigned short occl_update_counter = 0;

public:

	~TCompAudioEmitter();

	void onEntityCreated();
	void onAllEntitiesCreated(const TMsgAllEntitiesCreated& msg);
	void load(const json& j, TEntityParseContext& ctx);
	void update(float dt);
	void debugInMenu();
	
	// Functions to pause and resume emission
	void stopEmitter();
	void playEmitter();

	static void registerMsgs() {
		DECL_MSG(TCompAudioEmitter, TMsgAllEntitiesCreated, onAllEntitiesCreated);
	}
};
