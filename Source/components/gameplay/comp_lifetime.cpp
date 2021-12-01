#include "mcv_platform.h"
#include "comp_lifetime.h"
#include "entity/entity.h"
#include "engine.h"
#include "lua/module_scripting.h"
#include "components/common/comp_render.h"
#include "components/common/comp_parent.h"
#include "components/render/comp_dissolve.h"

DECL_OBJ_MANAGER("lifetime", TCompLifetime)

void TCompLifetime::load(const json& j, TEntityParseContext& ctx)
{
	_ttl = j.value("ttl", _ttl);
	_enabled = j.value("enabled", _enabled);
	_event_callback = j.value("event_callback", _event_callback);
}

void TCompLifetime::onEntityCreated()
{
	if (_enabled)
		init();
}

void TCompLifetime::init()
{
	if (_executed)
		return;

	static int idx = 0;
	CEntity* owner = getEntity();
	std::string name = owner->getName();
	owner->setName((name + std::to_string(++idx)).c_str());
	name = owner->getName();

	std::string argument;
	if (!_event_callback.empty()) {
		argument = "dispatchEvent('" + _event_callback + "')\n";
	}

	argument += "destroyEntity('" + name + "')";

	_executed = EngineLua.executeScript(argument, _ttl);
	assert(_executed);

	TCompDissolveEffect* c_dissolve = owner->get<TCompDissolveEffect>();

	// if this wasn't enabled, it's because we could't compute
	// before the wait time
	if (c_dissolve && !_enabled) {
		c_dissolve->fromLifetime(_ttl);
	}

	TCompParent* c_parent = owner->get<TCompParent>();
	if (c_parent) {
		c_parent->forEachChild([&](CHandle h) {
			CEntity* e = h;
			TCompDissolveEffect* c_dissolve = e->get<TCompDissolveEffect>();
			if (c_dissolve && !_enabled) {
				c_dissolve->fromLifetime(_ttl);
			}
		});
	}
}