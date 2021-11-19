#include "mcv_platform.h"
#include "engine.h"
#include "components/audio/comp_audio_emitter.h"
#include "components/common/comp_transform.h"
#include "audio/module_audio.h"

DECL_OBJ_MANAGER("audio_emitter", TCompAudioEmitter)

void TCompAudioEmitter::load(const json& j, TEntityParseContext& ctx)
{
	event_name = j.value("fmod_event", std::string());
	bank_name = j.value("fmod_bank", std::string());
	static_inst = j.value("static", false);
}

TCompAudioEmitter::~TCompAudioEmitter()
{
	event_inst->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
	event_inst->release();
}

void TCompAudioEmitter::onEntityCreated()
{
	assert(!event_name.empty());

	// Cache frequent components (used in update)
	h_cache_transform = get<TCompTransform>();

	// Load bank if necessary
	if (!bank_name.empty()) 
		EngineAudio.loadBank(bank_name);

	// Spawn event
	TCompTransform* t = h_cache_transform;
	event_inst = EngineAudio.postEventGetInst(event_name, t->getPosition());
}

void TCompAudioEmitter::update(float dt)
{
	// Update emitter world position
	if (event_inst == nullptr || static_inst)
		return;

	TCompTransform* t = h_cache_transform;
	FMOD_3D_ATTRIBUTES spacialization = {
		VEC3_TO_FMODVEC3(t->getPosition()),
		FMOD_VECTOR{0, 0, 0},
		VEC3_TO_FMODVEC3(t->getForward()),
		VEC3_TO_FMODVEC3(t->getUp())
	};

	event_inst->set3DAttributes(&spacialization);
}

void TCompAudioEmitter::debugInMenu()
{
}
