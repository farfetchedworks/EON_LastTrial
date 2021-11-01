#pragma once
#include "entity/entity.h"
#include "components/common/comp_base.h"
#include "components/messages.h"

struct TAttribute {
	unsigned short int value;
	unsigned short int ratio;
};

class TCompAttributes : public TCompBase {

	DECL_SIBLING_ACCESS();

private:

	unsigned short int initial_value = 1;
	CHandle h_controller;
	int phase_helper = 1;

	std::unordered_map <std::string, TAttribute> attributes;

	void add(std::string name, unsigned short int ratio = 1);
	TAttribute get(const std::string& name);

	void loadRatios(const json& j);
	void increment(const std::string& name);
	void setRatio(const std::string& name, unsigned short int ratio);

	void updateVitality();
	void updateCoherence();
	void updateEndurance();

public:

	void load(const json& j, TEntityParseContext& ctx);
	void onEntityCreated();
	void debugInMenu();

	int computeDamageSent(int base_dmg);
	int computeDamageReceived(int base_dmg);
	void onNewPhase(int phase);
	void forEach(std::function<void(const std::string, unsigned short int)> fn);
};
