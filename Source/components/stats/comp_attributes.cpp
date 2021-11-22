#include "mcv_platform.h"
#include "handle/handle.h"
#include "comp_attributes.h"
#include "entity/entity.h"
#include "components/controllers/comp_player_controller.h"

#include "components/stats/comp_health.h"
#include "components/stats/comp_warp_energy.h"
#include "components/stats/comp_stamina.h"

DECL_OBJ_MANAGER("attributes", TCompAttributes)

void TCompAttributes::load(const json& j, TEntityParseContext& ctx)
{
	initial_value = j.value("initial_value", initial_value);

	// name, increase ratio
	add("strength", 5);
	add("vitality", 5);
	add("armor");
	add("coherence");
	add("endurance", 10);

	if(j.count("ratios"))
		loadRatios(j["ratios"]);
}

void TCompAttributes::onEntityCreated()
{
	h_controller = get<TCompPlayerController>();
}

void TCompAttributes::loadRatios(const json& j)
{
	assert(j.is_array());

	for (int i = 0; i < j.size(); ++i) {

		const json& j_item = j[i];
		assert(j_item.is_object());

		if (!j_item.count("name")) continue;

		unsigned short int ratio = j_item.value("ratio", 1);
		setRatio(j_item["name"], ratio);
	}
}

void TCompAttributes::increment(const std::string& name)
{
	if (attributes.find(name) == attributes.end()) {
		dbg("Can't set value to attribute %s!\n", name);
		return;
	}

	attributes[name].value = attributes[name].value + 1;

#ifdef _DEBUG
	dbg("%s updated!\n", name.c_str());
#endif
}

void TCompAttributes::setRatio(const std::string& name, unsigned short int ratio)
{
	if (attributes.find(name) == attributes.end()) {
		dbg("Can't set ratio to attribute %s!\n", name);
		return;
	}

	attributes[name].ratio = ratio;
}

void TCompAttributes::add(std::string name, unsigned short int ratio)
{
	if (attributes.find(name) != attributes.end())
		return;

	attributes[name] = { initial_value, ratio };
}

TAttribute TCompAttributes::get(const std::string& name)
{
	if (attributes.find(name) != attributes.end())
		return attributes[name]; 

	dbg("Attribute %s not found!\n", name.c_str());
	return TAttribute({ 0, 0 });
}

int TCompAttributes::computeDamageSent(int base_dmg)
{
	TAttribute attr = get("strength");
	return base_dmg + (attr.value - 1) * attr.ratio;
}

int TCompAttributes::computeDamageReceived(int base_dmg)
{
	float magic = 12.f;
	TAttribute attr = get("armor");
	return (int)(base_dmg * (1.f - attr.value / magic));
}

void TCompAttributes::updateVitality()
{
	increment("vitality");

	TCompPlayerController* controller = h_controller;
	TCompHealth* health = get<TCompHealth>();
	assert(controller && health);

	TAttribute attr = get("vitality");
	int base_health = 100;
	int max_health = base_health + (attr.value - 1) * attr.ratio;
	health->setCurrMaxHealth(max_health);

	// Alex: I think for health its logic 
	// (stamina and warp can be recovered easily)
	health->fillHealth();
}

void TCompAttributes::updateCoherence()
{
	increment("coherence");

	TCompPlayerController* controller = h_controller;
	TCompWarpEnergy* warp = get<TCompWarpEnergy>();
	assert(controller && warp);

	TAttribute attr = get("coherence");
	int base_slots = 6;
	int max_warp = base_slots + (int)(floor((attr.value - 1) / 2));
	
	// better this?
	max_warp = base_slots + (attr.value - 1);

	warp->setCurrMaxWarp(max_warp);
}


void TCompAttributes::updateEndurance()
{
	increment("endurance");

	TCompPlayerController* controller = h_controller;
	TCompStamina* stamina = get<TCompStamina>();
	assert(controller && stamina);

	TAttribute attr = get("endurance");
	int base_stamina = 100;
	int max_stamina = base_stamina + (attr.value - 1) * attr.ratio;

	stamina->setMaxStamina(max_stamina);
}

void TCompAttributes::onNewPhase(int phase)
{
	// even/odd phases
	if (phase % 2 == 0) increment("strength");
	else updateVitality();

	// every 3 phases
	phase_helper = (phase_helper + 1) % 3;
	if (phase_helper == 0) increment("armor");
	else if (phase_helper == 1) updateCoherence();
	else if (phase_helper == 2) updateEndurance();
}

void TCompAttributes::forEach(std::function<void(const std::string, unsigned short int)> fn)
{
	for (auto& att : attributes)
	{
		fn(att.first, att.second.value);
	}
}

void TCompAttributes::debugInMenu()
{
	for (auto& att : attributes)
	{
		ImGui::Text("Attr: %s", att.first.c_str());
		ImGui::Text("Value: %d", att.second.value);
		ImGui::SameLine();
		ImGui::Text("Ratio: %d", att.second.ratio);
		ImGui::Separator();
	}
}
