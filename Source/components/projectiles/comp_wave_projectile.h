#pragma once
#include "components/common/comp_base.h"
#include "entity/entity.h"
#include "components/messages.h"

class TCompWaveProjectile : public TCompBase {

	DECL_SIBLING_ACCESS();

	friend class TCompAreaDelay;

	enum class EWaveCaster {
		ENEMY = 0,
		PLAYER
	};

private:

	float duration = 0.f;
	float radius = 0.f;
	float max_radius = 0.f;
	float current_time = 0.f;
	float speed_reduction = 0.f;

	CHandle light_source;
	float light_source_intensity = 0.0f;
	float wave_intensity = 0.0f;

	CHandle h_transform;
	CHandle currentDebuf;

	float* fluid_intensity = nullptr;
	int fluid_id = -1;

	EWaveCaster wave_caster = EWaveCaster::ENEMY;

	void setParameters(float wave_duration, float casting_radius, float max_casting_radius, bool byPlayer = false);
	void changeWaveRadius(float new_radius);
	void destroyWave();

public:

	void load(const json& j, TEntityParseContext& ctx);
	void onEntityCreated();
	void update(float dt);
	void debugInMenu();

	void activate();
	static void renderAll(CTexture* diffuse);
};
