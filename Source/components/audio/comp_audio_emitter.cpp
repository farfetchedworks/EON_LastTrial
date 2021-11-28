#include "mcv_platform.h"
#include "engine.h"
#include "components/audio/comp_audio_emitter.h"
#include "components/common/comp_transform.h"
#include "components/controllers/comp_player_controller.h"
#include "audio/module_audio.h"
#include "modules/module_physics.h"

DECL_OBJ_MANAGER("audio_emitter", TCompAudioEmitter)

void TCompAudioEmitter::load(const json& j, TEntityParseContext& ctx)
{
	event_name = j.value("fmod_event", std::string());
	bank_name = j.value("fmod_bank", std::string());
	static_inst = j.value("static", false);
	updates_occl = j.value("updates_occlusion", true);
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

void TCompAudioEmitter::onAllEntitiesCreated(const TMsgAllEntitiesCreated& msg)
{
	CEntity* e_player = getEntityByName("player");
	h_cache_player_transform = e_player->get<TCompTransform>();
}

bool hasObstaclesToEonIgnoreNavmesh(TCompTransform* my_trans, TCompTransform* player_trans, physx::PxU32 mask)
{
	// If Eon is inside the cone, generate a raycast from the Enemy towards Eon's position
	VEC3 raycast_origin = my_trans->getPosition();
	VEC3 dir = player_trans->getPosition() - raycast_origin;

	dir.Normalize();
	VHandles colliders;
	raycast_origin.y += 1;
	float distance = 200.f;

	bool is_ok = EnginePhysics.raycast(raycast_origin, dir, distance, colliders, mask, false, true);
	if (is_ok) {
		TCompCollider* c_collider;
		for (auto& c_collider : colliders) {
			CEntity* h_hit = c_collider.getOwner();
			std::string str = h_hit->getName();

			// if it collides with forbiddnen navmesh, ignore...
			if (!str.compare("collapsed_parent")) {
				continue;
			}

			// else, check
			if (h_hit == getEntityByName("player") || h_hit == getEntityByName("entrance")) {
				return false;
			}
			else
				return true;
		}
	}

	return true;
}

void TCompAudioEmitter::update(float dt)
{
	// Update emitter world position
	if (event_inst == nullptr || !h_cache_transform.isValid())
		return;

	TCompTransform* t = h_cache_transform;

	// If the emitter is sensitive to occlusion/obstruction
	if (updates_occl && h_cache_player_transform.isValid()) {
		TCompTransform* player_t = h_cache_player_transform;
		bool occluded = hasObstaclesToEonIgnoreNavmesh(t, player_t, CModulePhysics::FilterGroup::Scenario | CModulePhysics::FilterGroup::Player | CModulePhysics::FilterGroup::Prop | CModulePhysics::FilterGroup::InvisibleWall);

		if (occluded)
			event_inst->setParameterByName("Occluded", 1.f);
		else
			event_inst->setParameterByName("Occluded", 0.f);
	}
	
	// If the emitter is mobile
	if (!static_inst) {
		// Update emitter world position
		FMOD_3D_ATTRIBUTES spacialization = {
			VEC3_TO_FMODVEC3(t->getPosition()),
			FMOD_VECTOR{0, 0, 0},
			VEC3_TO_FMODVEC3(t->getForward()),
			VEC3_TO_FMODVEC3(t->getUp())
		};

		event_inst->set3DAttributes(&spacialization);
	}
}

void TCompAudioEmitter::stopEmitter()
{
	event_inst->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
}

void TCompAudioEmitter::playEmitter()
{
	event_inst->start();
}


void TCompAudioEmitter::debugInMenu()
{
}
