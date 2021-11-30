#pragma once
#include "components/common/comp_base.h"
#include "components/messages.h"

class TCompDissolveEffect : public TCompBase
{
	DECL_SIBLING_ACCESS();

	float _dissolveTime = 0.f;
	float _timer = 0.f;
	float _waitTimer = 0.f;
	bool _enabled = false;
	bool _useDefault = false;
	bool _removeColliders = true;
	bool _recovering = false;
	bool _inversed = false;
	bool _soundOn = true;

	std::string _originalMatName;

	void removeColliders();
	void applyDissolveMaterial();

public:
	void load(const json& j, TEntityParseContext& ctx);
	void update(float dt);
	void debugInMenu();
	void onEntityCreated();

	bool isEnabled();

	void enable(float time, float waitTime = 1e-6f, bool inversed = false, bool propagate_childs = true);
	void forceEnabled(bool enabled);
	void setMaterial(const std::string& mat_name);
	void setOriginalMaterial(const std::string& mat_name);
	const std::string& getOriginalMaterial();
	void setUseDefaultMat(bool use_default);
	void fromLifetime(float ttl);
	void updateObjectCte(CShaderCte<CtesObject>& cte);
	void recover(float time, float waitTime = 1e-6f, bool propagate_childs = true);
	//void recover(bool propagate_childs = true);
	void setRemoveColliders(bool remove, bool propagate_childs = true);
	void setSoundOn(bool val) { _soundOn = val; };
	void reset();

	float getDissolveTime() { return _dissolveTime; }
};
