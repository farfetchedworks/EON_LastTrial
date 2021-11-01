#include "mcv_platform.h"
#include "comp_emissive_blink.h"
#include "engine.h"
#include "lua/module_scripting.h"
#include "components/common/comp_render.h"

DECL_OBJ_MANAGER("emissive_blink", TCompEmissiveBlink);

void TCompEmissiveBlink::load(const json& j, TEntityParseContext& ctx)
{
	_unitPeriod = j.value("unitPeriod", _unitPeriod);
	_useOriginal = j.value("useOriginal", _useOriginal);
	_randomize = j.value("randomize", _randomize);
	_emissiveValue = j.value("emissiveValue", _emissiveValue);

	std::string original_chars = j.value("charString", std::string());
	assert(original_chars.size() > 0);

	// add internal pauses to simulate blinking
	for (int i = 0; i < original_chars.size(); ++i) {
		_chars += original_chars[i];
		if(!std::isspace(original_chars[i]))
			_chars += ' ';
	}
}

void TCompEmissiveBlink::onEntityCreated()
{
	CEntity* owner = getEntity();
	TCompRender* c_render = owner->get<TCompRender>();
	CMaterial* mat = const_cast<CMaterial*>(c_render->draw_calls[0].material);
	_originalEmissiveValue = mat->getEmissive();

	updateEmissive();
}

void TCompEmissiveBlink::updateEmissive()
{
	CEntity* owner = getEntity();
	TCompRender* c_render = owner->get<TCompRender>();
	CMaterial* mat = const_cast<CMaterial*>(c_render->draw_calls[0].material);

	assert(_currentChar < _chars.size());
	const char c = _chars[_currentChar];
	float value = (std::isspace(c) || c == 'x') ? 0.f : 1.f;

	if (_useOriginal) {
		value *= _originalEmissiveValue;
	}
	else {
		value *= (_emissiveValue + (_randomize ? Random::range(0.0f, _emissiveValue) : 0.f));
	}

	mat->setEmissive(value);
}

void TCompEmissiveBlink::update(float dt)
{
	_timer += dt;

	// Check if i have to go to next char
	{
		float charDuration = _unitPeriod;

		if (_chars[_currentChar] == '-' || std::isspace(_chars[_currentChar])) {
			charDuration *= 3;
		}

		if (_timer > charDuration) {
			_currentChar++;
			_currentChar = _currentChar % _chars.size();
			_timer = 0.f;
			updateEmissive();
		}
	}
}