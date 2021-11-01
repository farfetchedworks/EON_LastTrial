#pragma once

#include "components/common/comp_base.h"
#include "entity/entity.h"
#include "components/messages.h"

namespace FMOD {
	namespace Studio {
		class EventInstance;
	}
}

class TCompAreaDelayProjectile : public TCompBase {

	DECL_SIBLING_ACCESS();

	friend class TCompAreaDelay;						// Area delay component must define the parameters of the projectile

private:

	float area_duration			= 5.f;
	float radius				= 5.f;
	float speed_reduction		= 5.f;
	float current_time			= 0.f;					// current time for the timer for area delay

	float* fluid_intensity		= nullptr;
	int fluid_id				= -1;

	CHandle projectile_transform;
	CHandle light_source;
	float light_source_intensity = 0.0f;
	float area_intensity = 0.0f;

	// Stored audio event, to kill whenever the branch destroys itself
	FMOD::Studio::EventInstance* fmod_event = nullptr;

	/*
		Applies area delay to an object. Returns true if the object has a controller
		with an associated speed
	*/ 
	void applyAreaDelay(const TMsgEntityTriggerEnter& msg);
	
	// Removes the delay effect on enemies that are outside the area	
	void removeDelayEffect(const TMsgEntityTriggerExit& msg);

	// Destroys the area projectile and removes the effect on all affected objects
	void destroyAreaDelay();

	void setParameters(float area_duration, float radius, float speed_reduction);
	void changeSphereRadius(float new_radius);

public:

	float getSpeedReduction() { return speed_reduction; }

	static void registerMsgs() {
		DECL_MSG(TCompAreaDelayProjectile, TMsgEntityTriggerEnter, applyAreaDelay);
		DECL_MSG(TCompAreaDelayProjectile, TMsgEntityTriggerExit, removeDelayEffect);
	}

	//void load(const json& j, TEntityParseContext& ctx);
	void onEntityCreated();
	void update(float dt);
	void debugInMenu();
	void renderDebug();
	void activate();

	static void renderAll(CTexture* diffuse);
};
