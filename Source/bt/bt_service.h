#pragma once

#include "bt_common.h"

class CBTContext;
class CBTTaskFactory;

class IBTService
{
protected:
	//std::string service_name;
	float period_acum = 0.f;

public:
	float period = 1.f;

	////////////////////////////////////////////
	// Generic parameters loaded from OWLBT. Each task will use them in for its own purpose.
	float number_field = 0.f;
	std::string string_field = std::string();
	////////////////////////////////////////////

	virtual EBTNodeResult executeService(CBTContext& ctx, float dt) { return EBTNodeResult::SUCCEEDED; }
	virtual void init() {}
	EBTNodeResult tickService(CBTContext& ctx, float dt);

	// Called when the service is entered
	virtual void onEnter(CBTContext& ctx);

	std::string name;
	std::string_view type;

};

using CBTServiceDummy = IBTService;