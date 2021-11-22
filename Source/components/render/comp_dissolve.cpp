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

void TCompDissolveEffect::removeColliders()
{
	// Remove collider if doesn't have BT
	// In that case, the BT does the work
	CEntity* owner = getEntity();
	TCompBT* my_bt = get<TCompBT>();
	if (!my_bt && _removeColliders) {
		owner->removeComponent<TCompCollider>();
		owner->removeComponent<TCompUpdatesPhysics>();
	}
}

void TCompDissolveEffect::load(const json& j, TEntityParseContext& ctx)
{
	_waitTimer = j.value("wait_time", _waitTimer);
	_dissolveTime = j.value("duration", _dissolveTime);
	_useDefault = j.value("useDefault", _useDefault);
	_removeColliders = j.value("removeCollider", _removeColliders);
	_enabled = j.value("enabled", _enabled);
}

void TCompDissolveEffect::debugInMenu()
{
	ImGui::Checkbox("Enabled", &_enabled);
	ImGui::Checkbox("Recovering", &_recovering);
	ImGui::Text("Timer: %f", _timer);
	ImGui::Text("Original: %s", _originalMatName.c_str());
}

void TCompDissolveEffect::onEntityCreated()
{
	if (_enabled) {
		enable(_dissolveTime, _waitTimer);
	}
}

void TCompDissolveEffect::fromLifetime(float ttl)
{
	_waitTimer = std::max(ttl - _dissolveTime, 0.f);
}

void TCompDissolveEffect::enable(float time, float waitTime, bool inversed, bool propagate_childs)
{
	_waitTimer = waitTime;
	_dissolveTime = time;
	_inversed = inversed;

	applyDissolveMaterial();

	_enabled = true;

	if (_inversed) {
		_timer = _dissolveTime;
	}
	
	TCompParent* c_parent = get<TCompParent>();
	if (c_parent && propagate_childs) {
		c_parent->forEachChild([time, waitTime, inversed](CHandle h) {
			CEntity* e = h;
			TCompDissolveEffect* c_dissolve = e->get<TCompDissolveEffect>();
			if (c_dissolve) {
				c_dissolve->enable(time, waitTime, inversed);
			}
		});
	}
}

void TCompDissolveEffect::forceEnabled(bool enabled)
{
	_enabled = enabled;
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

void TCompDissolveEffect::setUseDefaultMat(bool use_default)
{
	_useDefault = use_default;
}

void TCompDissolveEffect::applyDissolveMaterial()
{
	CEntity* owner = getEntity();
	TCompRender* c_render = owner->get<TCompRender>();
	const std::string& material_name = c_render->draw_calls[0].material->getName();

	// Dissolve already applied
	if (!_originalMatName.empty() && _originalMatName != material_name) {
		return;
	}

	setMaterial(_useDefault ? "data/materials/dissolve.mat" : addSuffixBeforeExtension(material_name, "_dissolve"));

	_originalMatName = material_name;

	// FMOD event
	static const char* EVENT_NAME = "ENV/General/Disintegrate";
	EngineAudio.postEvent(EVENT_NAME, CHandle(this).getOwner());
}

void TCompDissolveEffect::recover(float time, float waitTime, bool propagate_childs)
{
	//if (_removeColliders || !_originalMatName.length())
	//	return;

	_dissolveTime = time;
	_waitTimer = waitTime;
	_recovering = true;
	_enabled = true;

	applyDissolveMaterial();

	TCompParent* c_parent = get<TCompParent>();
	if (c_parent && propagate_childs) {
		c_parent->forEachChild([&](CHandle h) {
			CEntity* e = h;
			TCompDissolveEffect* c_dissolve = e->get<TCompDissolveEffect>();
			if (c_dissolve) {
				c_dissolve->recover(time, waitTime, propagate_childs);
			}
		});
	}
}

//void TCompDissolveEffect::recover(bool propagate_childs)
//{
//	//if (_removeColliders || !_originalMatName.length())
//	//	return;
//
//	_recovering = true;
//
//	TCompParent* c_parent = get<TCompParent>();
//	if (c_parent && propagate_childs) {
//		c_parent->forEachChild([&](CHandle h) {
//			CEntity* e = h;
//			TCompDissolveEffect* c_dissolve = e->get<TCompDissolveEffect>();
//			if (c_dissolve) {
//				c_dissolve->recover(propagate_childs);
//			}
//		});
//	}
//}

void TCompDissolveEffect::setRemoveColliders(bool remove, bool propagate_childs)
{
	_removeColliders = remove;

	TCompParent* c_parent = get<TCompParent>();
	if (c_parent && propagate_childs) {
		c_parent->forEachChild([&](CHandle h) {
			CEntity* e = h;
			TCompDissolveEffect* c_dissolve = e->get<TCompDissolveEffect>();
			if (c_dissolve) {
				c_dissolve->setRemoveColliders(remove, propagate_childs);
			}
		});
	}
}

void TCompDissolveEffect::update(float dt)
{
	if (!_enabled)
		return;

	if (_waitTimer > 0.f)
	{
		_waitTimer -= dt;

		if (_waitTimer < 0.f)
		{
			if (_removeColliders) {
				removeColliders();
			}
		}
		else {
			return;
		}
	}

	TCompGameManager* gm = GameManager->get<TCompGameManager>();
	dt *= gm->getTimeScaleFactor();

	if (_inversed) {
		if (_recovering) {
			_timer += dt;
		}
		else {
			_timer -= dt;
		}
	}
	else {
		if (_recovering) {
			_timer -= dt;
		}
		else {
			_timer += dt;
		}
	}

	if (_timer < 0.0f || _timer > _dissolveTime) {
		reset();
	}
}

void TCompDissolveEffect::reset()
{
	_dissolveTime = 0.f;
	_waitTimer = 0.f;
	_timer = _inversed ? _dissolveTime : 0.0f;
	_recovering = false;
	_enabled = false;
	setMaterial(_originalMatName);
}