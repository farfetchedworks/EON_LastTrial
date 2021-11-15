#include "mcv_platform.h"
#include "entity/entity.h"
#include "components/common/comp_base.h"
#include "components/messages.h"
#include "fmod_studio.hpp"

class TCompAudioEmitter : public TCompBase {

	DECL_SIBLING_ACCESS();

private:

	CHandle h_cache_transform;

	std::string event_name;
	std::string bank_name;
	FMOD::Studio::EventInstance* event_inst;

public:

	~TCompAudioEmitter();

	void onEntityCreated();
	void load(const json& j, TEntityParseContext& ctx);
	void update(float dt);
	void debugInMenu();
};
