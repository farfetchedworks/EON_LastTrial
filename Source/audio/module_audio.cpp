#include "mcv_platform.h"
#include "engine.h"
#include "module_audio.h"
#include "fmod.hpp"
#include "fmod_studio.hpp"
#include "modules/module_physics.h"
#include "components/common/comp_render.h"
#include "components/common/comp_fsm.h"
#include "components/controllers/comp_pawn_controller.h"
#include "components/controllers/comp_player_controller.h"
#include "audio/physical_material.h"
#include "entity/entity.h"
#include "handle/handle.h"

using namespace FMOD;

static const char* ENCRYPTION_KEY = "8Ukgpaspqa6&R*3Ji4ma#Gt!Yb&";
static const std::string AUDIO_PATH = "data/audio/";
static const std::string EVENT_PREFIX = "event:/";
static const std::string SW_SURFACE_RTPC = "SW_Surface";

class myFMOD_RESULT 
{
	FMOD_RESULT res;

public:
	myFMOD_RESULT(FMOD_RESULT _res) : res(_res) {}

	myFMOD_RESULT& operator=(const FMOD_RESULT& other) {
		res = other;
		return *this;
	}

	bool operator==(const FMOD_RESULT& other) {
		return res == other;
	}

	myFMOD_RESULT& operator&=(const FMOD_RESULT& other) {
		if (other != FMOD_OK) {
			res = FMOD_ERR_INTERNAL;
		}
		return *this;
	}

	myFMOD_RESULT& operator&=(const myFMOD_RESULT& other) {
		if (other.res != FMOD_OK) {
			res = FMOD_ERR_INTERNAL;
		}
		return *this;
	}
};

myFMOD_RESULT CModuleAudio::createEventInstance(const std::string& event_name, Studio::EventInstance** event_inst) const
{
	myFMOD_RESULT is_ok = FMOD_RESULT::FMOD_OK;

	// Get EventDescription from system
	Studio::EventDescription* ev_descr;
	is_ok &= system->getEvent((EVENT_PREFIX + event_name).c_str(), &ev_descr);

	// Disable doppler (unnecessary)
	bool doppler_state = false;
	is_ok &= ev_descr->isDopplerEnabled(&doppler_state);

	// Create EventInstance from EventDescription
	is_ok &= ev_descr->createInstance(event_inst);

	return is_ok;
}

myFMOD_RESULT CModuleAudio::create3DEventInstance(const std::string& event_name, Studio::EventInstance** event_inst, VEC3 pos) const
{
	myFMOD_RESULT is_ok = FMOD_RESULT::FMOD_OK;

	// Create an instance for the event
	is_ok &= createEventInstance(event_name, event_inst);

	// Set location
	FMOD_3D_ATTRIBUTES spacialization = {
		VEC3_TO_FMODVEC3(pos),
		FMOD_VECTOR{0, 0, 0},
		FMOD_VECTOR{0, 0, 1},
		FMOD_VECTOR{0, 1, 0}
	};

	is_ok &= (*event_inst)->set3DAttributes(&spacialization);

	return is_ok;
}

myFMOD_RESULT CModuleAudio::create3DEventInstance(const std::string& event_name, Studio::EventInstance** event_inst, CHandle actor) const
{
	myFMOD_RESULT is_ok = FMOD_RESULT::FMOD_OK;

	CEntity* e_actor = actor;

	// Create an instance for the event
	is_ok &= createEventInstance(event_name, event_inst);

	// If the actor has a collider, retrieve speed from it
	VEC3 velocity = VEC3(0, 0, 0);
	TCompCollider* collider = e_actor->get<TCompCollider>();
	if (collider) {
		velocity = collider->getLinearVelocity();
	}

	// Set location
	FMOD_3D_ATTRIBUTES spacialization = {
		VEC3_TO_FMODVEC3(e_actor->getPosition()),
		VEC3_TO_FMODVEC3(velocity),
		VEC3_TO_FMODVEC3(e_actor->getForward()),
		VEC3_TO_FMODVEC3(e_actor->getUp())
	};
	is_ok &= (*event_inst)->set3DAttributes(&spacialization);

	return is_ok;
}

////////////////////
// Public methods //
////////////////////

bool CModuleAudio::start()
{
	myFMOD_RESULT is_ok = FMOD_RESULT::FMOD_OK;

	// Create FMOD Studio system
	is_ok &= Studio::System::create(&system);

	// Validate encryption key
	FMOD_STUDIO_ADVANCEDSETTINGS settings = {
		sizeof(FMOD_STUDIO_ADVANCEDSETTINGS),
		32768,
		8192 * sizeof(void*),
		20,
		262144,
		8192,
		ENCRYPTION_KEY
	};
	FMOD_RESULT asd = system->setAdvancedSettings(&settings);
	system->getAdvancedSettings(&settings);

	// Initialise FMOD Studio system
	is_ok &= system->initialize(1024, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_3D_RIGHTHANDED, nullptr);

	// Set number of listeners influencing the mix (always 1, either it's Eon, a camera...)
	is_ok &= system->setNumListeners(1);

	// Load master banks
	loadBank("Master.bank");
	loadBank("Master.strings.bank");

	assert(is_ok == FMOD_RESULT::FMOD_OK); // all ok
	return is_ok == FMOD_RESULT::FMOD_OK;
}

void CModuleAudio::stop()
{
	system->unloadAll();
	system->release();
}

void CModuleAudio::update(float dt)
{
	// Update the transform of the listener, if it exists
	if (listener.isValid()) {
		CEntity* e_listener = listener;
		const FMOD_VECTOR listener_pos = VEC3_TO_FMODVEC3(e_listener->getPosition());

		// Set speed of Eon
		/*
			CHandle h_eon = getEntityByName("player");
			VEC3 vel = VEC3(0, 0, 0);
			if (h_eon.isValid()) {
				CEntity* e_eon = h_eon;
				TCompCollider* collider = e_eon->get<TCompCollider>();
				vel = collider->getLinearVelocity();
			}
		*/

		FMOD_3D_ATTRIBUTES attributes = {
			listener_pos,
			FMOD_VECTOR{}, //VEC3_TO_FMODVEC3(vel) : right now its unnecessary, as it (presumably) only affects doppler
			VEC3_TO_FMODVEC3(e_listener->getForward()),
			VEC3_TO_FMODVEC3(e_listener->getUp()),
		};
		system->setListenerAttributes(0, &attributes, &listener_pos);
	}
	
	// Update the system
	system->update();
}

void CModuleAudio::renderInMenu()
{

}

bool CModuleAudio::loadBank(const std::string& bank_name)
{
	myFMOD_RESULT is_ok = system->loadBankFile((AUDIO_PATH + bank_name).c_str(), FMOD_STUDIO_LOAD_BANK_NORMAL, &soundbanks[bank_name]);
	return is_ok == FMOD_RESULT::FMOD_OK;
}

bool CModuleAudio::unloadBank(const std::string& bank_name)
{
	myFMOD_RESULT is_ok = soundbanks[bank_name]->unload();
	return is_ok == FMOD_RESULT::FMOD_OK;
}

bool CModuleAudio::postEvent(const std::string& event_name)
{
	myFMOD_RESULT is_ok = FMOD_RESULT::FMOD_OK;

	// Create an instance for the event
	Studio::EventInstance* ev_inst;
	is_ok &= createEventInstance(event_name, &ev_inst);

	// Fire instance
	is_ok &= ev_inst->start();
	is_ok &= ev_inst->release();

	return is_ok == FMOD_RESULT::FMOD_OK;
}

bool CModuleAudio::postEvent(const std::string& event_name, const VEC3 location)
{
	myFMOD_RESULT is_ok = FMOD_RESULT::FMOD_OK;

	// Create an instance for the event
	Studio::EventInstance* ev_inst;
	is_ok &= create3DEventInstance(event_name, &ev_inst, location);

	// Fire instance
	is_ok &= ev_inst->start();
	is_ok &= ev_inst->release();

	return is_ok == FMOD_RESULT::FMOD_OK;
}

bool CModuleAudio::postEvent(const std::string& event_name, CHandle actor)
{
	myFMOD_RESULT is_ok = FMOD_RESULT::FMOD_OK;

	if (!actor.isValid())
		return false;

	// Since we post it on an actor, we must mute it if the actor is slowed down
	CEntity* e_actor = actor;
	TCompPawnController* h_pawn = e_actor->get<TCompPawnController>();

	if (h_pawn && h_pawn->speed_multiplier < 1.f)
		return is_ok == FMOD_RESULT::FMOD_OK;
	
	// Create an instance for the event
	Studio::EventInstance* ev_inst;
	is_ok &= create3DEventInstance(event_name, &ev_inst, actor);

	// Fire instance
	is_ok &= ev_inst->start();
	is_ok &= ev_inst->release();

	// Store reference to the event inside the actor's FSM, as it must be killed
	// if the current state is changed to another (attack cancelation, etc)
	TCompFSM* h_fsm = e_actor->get<TCompFSM>();
	if (h_fsm)
		h_fsm->getCtx().setFmodEvent(ev_inst);

	return is_ok == FMOD_RESULT::FMOD_OK;
}

FMOD::Studio::EventInstance* CModuleAudio::post2DEventGetInst(const std::string& event_name)
{
	myFMOD_RESULT is_ok = FMOD_RESULT::FMOD_OK;

	// Create an instance for the event
	Studio::EventInstance* ev_inst;
	is_ok &= createEventInstance(event_name, &ev_inst);

	// Fire instance
	is_ok &= ev_inst->start();

	assert(is_ok == FMOD_RESULT::FMOD_OK);

	return ev_inst;
}

FMOD::Studio::EventInstance* CModuleAudio::postEventGetInst(const std::string& event_name, const VEC3 location)
{
	myFMOD_RESULT is_ok = FMOD_RESULT::FMOD_OK;

	// Create an instance for the event
	Studio::EventInstance* ev_inst;
	is_ok &= create3DEventInstance(event_name, &ev_inst, location);

	// Fire instance
	is_ok &= ev_inst->start();

	assert(is_ok == FMOD_RESULT::FMOD_OK);

	return ev_inst;
}

bool CModuleAudio::setGlobalRTPC(const std::string& param_name, const float value, const bool ignore_seek_speed)
{
	myFMOD_RESULT is_ok = FMOD_RESULT::FMOD_OK;
	
	// Set the parameter
	is_ok = system->setParameterByName(param_name.c_str(), value, ignore_seek_speed);

	return is_ok == FMOD_RESULT::FMOD_OK;
}

bool CModuleAudio::postFloorEvent(const std::string& event_name, CHandle actor)
{
	myFMOD_RESULT is_ok = FMOD_RESULT::FMOD_OK;

	if (!actor.isValid())
		return false;

	// Create an instance for the event
	Studio::EventInstance* ev_inst;
	is_ok &= create3DEventInstance(event_name, &ev_inst, actor);

	// Query material on the floor
	VEC3 floor_hit;
	const CPhysicalMaterial* phys_mat = EnginePhysics.getPhysicalMaterialOfFloor(actor, floor_hit);

	if (phys_mat) {
		// Set the RTPC "SW_Surface" of the event to be fired
		float phys_mat_ID = (float)phys_mat->type;
		is_ok &= ev_inst->setParameterByName(SW_SURFACE_RTPC.c_str(), phys_mat_ID);
	}
	
	// Tell the player
	{
		CEntity* player = getEntityByName("player");
		if (player) {
			TCompPlayerController* pawn = player->get<TCompPlayerController>();
			if (pawn && actor == CHandle(player)) {
				pawn->setFloorMaterialInfo(phys_mat, floor_hit);
			}
		}
	}

	// Fire instance
	is_ok &= ev_inst->start();
	is_ok &= ev_inst->release();

	return is_ok == FMOD_RESULT::FMOD_OK;
}

// Music event - specifics
void stopAndRelease(FMOD::Studio::EventInstance* ev_inst)
{
	if (ev_inst == nullptr)
		return;

	ev_inst->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
	ev_inst->release();
}

void CModuleAudio::postMusicEvent(const std::string& event_name)
{
	stopAndRelease(cur_mus_event);
	cur_mus_event = post2DEventGetInst(event_name);
}

bool CModuleAudio::setMusicRTPC(const std::string& param_name, const float value, const bool ignore_seek_speed)
{
	myFMOD_RESULT is_ok = FMOD_RESULT::FMOD_OK;

	if (cur_mus_event == nullptr)
		return false;

	is_ok &= cur_mus_event->setParameterByName(param_name.c_str(), value, ignore_seek_speed);
	return is_ok == FMOD_RESULT::FMOD_OK;
}

void CModuleAudio::stopCurMusicEvent()
{
	stopAndRelease(cur_mus_event);
}