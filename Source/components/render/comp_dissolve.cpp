#include "mcv_platform.h"
#include "comp_dissolve.h"
#include "engine.h"
#include "audio/module_audio.h"
#include "lua/module_scripting.h"
#include "components/common/comp_parent.h"
#include "components/common/comp_render.h"
#include "components/common/comp_collider.h"
#include "components/common/comp_updates_physics.h"
#include "components/ai/comp_bt.h"

DECL_OBJ_MANAGER("dissolve_effect", TCompDissolveEffect);

void TCompDissolveEffect::load(const json& j, TEntityParseContext& ctx)
{
	_waitTimer = j.value("wait_time", _waitTimer);
	_dissolveTime = j.value("duration", _dissolveTime);
	_useDefault = j.value("useDefault", _useDefault);
	_removeCollider = j.value("removeCollider", _removeCollider);
}

void TCompDissolveEffect::debugInMenu()
{
	ImGui::Checkbox("Enabled", &_enabled);
	ImGui::Checkbox("Recovering", &_recovering);
	ImGui::Text("Timer: %f", _timer);
	ImGui::Text("Original: %s", _originalMatName.c_str());
}

void TCompDissolveEffect::fromLifetime(float ttl)
{
	_waitTimer = std::max(ttl - _dissolveTime, 0.f);
}

void TCompDissolveEffect::enable(float time, float waitTime, bool propagate_childs)
{
	_waitTimer = waitTime;
	_dissolveTime = time;
	
	TCompParent* c_parent = get<TCompParent>();
	if (c_parent && propagate_childs) {
		c_parent->forEachChild([time, waitTime](CHandle h) {
			CEntity* e = h;
			TCompDissolveEffect* c_dissolve = e->get<TCompDissolveEffect>();
			if (c_dissolve) {
				c_dissolve->enable(time, waitTime);
			}
		});
	}
}

void TCompDissolveEffect::updateObjectCte(CShaderCte<CtesObject>& cte)
{
	if (!_enabled)
		return;

	cte.object_dissolve_timer = _timer;
	cte.object_max_dissolve_time = _dissolveTime;
}

void TCompDissolveEffect::setMaterial(const std::string& mat_name)
{
	CEntity* owner = getEntity();
	TCompRender* c_render = owner->get<TCompRender>();
	c_render->setMaterialForAll(Resources.get(mat_name)->as<CMaterial>());
}

void TCompDissolveEffect::applyDissolveMaterial()
{
	if (_enabled)
		return;

	CEntity* owner = getEntity();
	TCompRender* c_render = owner->get<TCompRender>();
	const std::string& material_name = c_render->draw_calls[0].material->getName();
	setMaterial(_useDefault ? "data/materials/dissolve.mat" : addSuffixBeforeExtension(material_name, "_dissolve"));

	_originalMatName = material_name;

	// FMOD event
	static const char* EVENT_NAME = "ENV/General/Disintegrate";
	EngineAudio.postEvent(EVENT_NAME, CHandle(this).getOwner());

	// Remove collider if doesn't have BT
	// In that case, the BT does the work
	TCompBT* my_bt = get<TCompBT>();
	if (!my_bt && _removeCollider) {
		owner->removeComponent<TCompCollider>();
		owner->removeComponent<TCompUpdatesPhysics>();
	}
}

void TCompDissolveEffect::recover(bool propagate_childs)
{
	assert(!_removeCollider);
	if (_removeCollider || !_originalMatName.length())
		return;

	_recovering = true;

	TCompParent* c_parent = get<TCompParent>();
	if (c_parent && propagate_childs) {
		c_parent->forEachChild([](CHandle h) {
			CEntity* e = h;
			TCompDissolveEffect* c_dissolve = e->get<TCompDissolveEffect>();
			if (c_dissolve) {
				c_dissolve->recover();
			}
		});
	}
}

void TCompDissolveEffect::update(float dt)
{
	if (_waitTimer > 0.f)
	{
		_waitTimer -= dt;

		if (_waitTimer < 0.f)
		{
			applyDissolveMaterial();
			_enabled = true;
		}
	}

	if (!_enabled)
		return;

	TCompGameManager* gm = GameManager->get<TCompGameManager>();
	dt *= gm->getTimeScaleFactor();

	if (_recovering) {

		TCompGameManager* gm = GameManager->get<TCompGameManager>();
		_timer -= dt;

		if (_timer <= 0.f) {
			reset();
		}
	}
	else {
		_timer += dt;
	}
}

void TCompDissolveEffect::reset()
{
	_dissolveTime = 0.f;
	_waitTimer = 0.f;
	_timer = 0.f;
	_recovering = false;
	_enabled = false;
	setMaterial(_originalMatName);
}