#pragma once
#include "modules/module.h"

#define VEC3_TO_FMODVEC3(vec3) FMOD_VECTOR{vec3.x, vec3.y, vec3.z}

class CHandle;
class CEntity;

namespace FMOD { 
	namespace Studio { 
		class System; 
		class Bank;
		class EventDescriptor;
		class EventInstance;
	};
};

class myFMOD_RESULT;

class CModuleAudio : public IModule {
private:
	FMOD::Studio::System* system = nullptr;

	std::unordered_map<std::string, FMOD::Studio::Bank*> soundbanks;
	
	CHandle listener;
	// std::unordered_map<std::string, FMOD::Studio::EventDescription*> cached_event_descriptors;

	FMOD::Studio::EventInstance* cur_mus_event = nullptr;

	myFMOD_RESULT createEventInstance(const std::string& event_name, FMOD::Studio::EventInstance** event_inst) const;
	myFMOD_RESULT create3DEventInstance(const std::string& event_name, FMOD::Studio::EventInstance** event_inst, VEC3 pos) const;
	myFMOD_RESULT create3DEventInstance(const std::string& event_name, FMOD::Studio::EventInstance** event_inst, CHandle actor) const;

public:
	CModuleAudio(const std::string& name) : IModule(name) { }

	bool start() override;
	void stop() override;
	void update(float delta) override;
	void renderInMenu() override;
	
	/*
	 * Loads a bank into memory, making its events accessible
	 */
	bool loadBank(const std::string& bank_name);

	/*
	 * Unloads a bank from memory, making its events inaccessible
	 */
	bool unloadBank(const std::string& bank_name);

	/*
	 * Posts an event without 3D-spacialization
	 */
	bool postEvent(const std::string& event_name);

	/*
	 * Posts an event at a certain location (for 3D-spacialization)
	 */
	bool postEvent(const std::string& event_name, const VEC3 location);

	/*
	 * Posts an event on a certain actor (for 3D-spacialization and RTPCs)
	 */
	bool postEvent(const std::string& event_name, CHandle actor);

	/*
	 * Posts a 2D event and returns its associated instance
	 */
	FMOD::Studio::EventInstance* post2DEventGetInst(const std::string& event_name);

	/*
	 * Posts an event on a certain actor (for 3D-spacialization and RTPCs) and returns its associated instance
	 */
	FMOD::Studio::EventInstance* postEventGetInst(const std::string& event_name, const VEC3 location);

	/*
	 * Sets the value of a real-time-parameter-control for an event instance
	 */
	bool setGlobalRTPC(const std::string& param_name, const float value, const bool ignore_seek_speed = false);

	/*
	 * Sets the actor that will be the listener, for 3D spacialization
	 */
	void setListener(CHandle actor) { listener = actor; };

	/*
	 * Posts an event on a certain actor modifyi
	 */
	bool postFloorEvent(const std::string& event_name, CHandle actor);

	/*
	 * Posts a music event and stores it as the music being played
	 */
	void postMusicEvent(const std::string& event_name);

	/*
	 * Posts a music event and stores it as the music being played, and returns it
	 */
	FMOD::Studio::EventInstance* getCurMusicEvent() { return cur_mus_event; }

	/*
	 * Sets the value of a real-time-parameter-control for the current music event
	 */
	bool setMusicRTPC(const std::string& param_name, const float value, const bool ignore_seek_speed = false);

	/*
	 * Stops the current music event being played
	 */
	void stopCurMusicEvent();

};
