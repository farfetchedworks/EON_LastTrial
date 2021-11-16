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
	bool _removeCollider = true;
	bool _recovering = false;

	std::string _originalMatName;

public:
	void load(const json& j, TEntityParseContext& ctx);
	void update(float dt);
	void debugInMenu();
	
	void enable(float time, float waitTime = 1e-6f, bool propagate_childs = true);
	void applyDissolveMaterial();
	void setMaterial(const std::string& mat_name);
	void fromLifetime(float ttl);
	void updateObjectCte(CShaderCte<CtesObject>& cte);
	void recover(bool propagate_childs = true);
	void setRemoveColliders(bool remove, bool propagate_childs = true);
	void reset();

	float getDissolveTime() { return _dissolveTime; }
};
