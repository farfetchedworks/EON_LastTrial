#include "mcv_platform.h"
#include "comp_emissive_modifier.h"
#include "engine.h"
#include "lua/module_scripting.h"
#include "components/common/comp_render.h"

DECL_OBJ_MANAGER("emissive_mod", TCompEmissiveMod);

void TCompEmissiveMod::load(const json& j, TEntityParseContext& ctx)
{
	_maxValue = j.value("maxValue", _maxValue);
	_inSpeed = j.value("inSpeed", _inSpeed);
	_outSpeed = j.value("outSpeed", _outSpeed);
}

void TCompEmissiveMod::onEntityCreated()
{
	CEntity* owner = getEntity();
	TCompRender* c_render = owner->get<TCompRender>();
	CMaterial* mat = const_cast<CMaterial*>(c_render->draw_calls[0].material);
	_minValue = _emissiveValue = mat->getEmissive();
	assert(_emissiveValue < _maxValue);
}

void TCompEmissiveMod::updateEmissive()
{
	CEntity* owner = getEntity();
	TCompRender* c_render = owner->get<TCompRender>();

	for (int i = 0; i < c_render->draw_calls.size(); ++i) {
		CMaterial* mat = const_cast<CMaterial*>(c_render->draw_calls[i].material);
		if (mat->getSkinMaterial())
			mat = const_cast<CMaterial*>(mat->getSkinMaterial());
		mat->setEmissive(_emissiveValue);
	}
}

void TCompEmissiveMod::blendIn()
{
	_blendIn = true;
	_blendOut = false;
}

void TCompEmissiveMod::blendOut()
{
	_blendIn = false;
	_blendOut = true;
}

void TCompEmissiveMod::update(float dt)
{
	bool changed = false;

	if (_blendIn) {
		_emissiveValue = damp(_emissiveValue, _maxValue, _inSpeed, dt);

		if (fabsf(_maxValue - _emissiveValue) < 0.001f)
			_blendIn = false;

		changed |= true;
	}
	else if (_blendOut) {
		_emissiveValue = damp(_emissiveValue, _minValue, _outSpeed, dt);

		if (fabsf(_emissiveValue - _minValue) < 0.001f)
			_blendOut = false;

		changed |= true;
	}

	if(changed)
		updateEmissive();
}

void TCompEmissiveMod::debugInMenu()
{
	ImGui::Text("Factor: %f", _emissiveValue);
	
	ImGui::Separator();

	if (ImGui::Button("Blend in")) {
		blendIn();
	}

	if (ImGui::Button("Blend out")) {
		blendOut();
	}
}