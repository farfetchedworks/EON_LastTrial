#pragma once

#include "bt_common.h"
#include "components/gameplay/comp_game_manager.h"

class CBTTaskFactory;
class CBTContext;

auto const null_lambda = [](CBTContext&, float) {};

struct SActionCallbacks {
	std::function<void(CBTContext&, float)> onFirstFrame = null_lambda;
	std::function<void(CBTContext&, float)> onStartup = null_lambda;
	std::function<void(CBTContext&, float)> onStartupFinished = null_lambda;
	std::function<void(CBTContext&, float)> onActive = null_lambda;
	std::function<void(CBTContext&, float)> onActiveFinished = null_lambda;
	std::function<void(CBTContext&, float)> onRecovery = null_lambda;
	std::function<void(CBTContext&, float)> onRecoveryFinished = null_lambda;
};

class IBTTask
{


protected:
	SActionCallbacks callbacks;

	void updateLogic(CBTContext& ctx, float dt);
	void cleanLogic(CBTContext& ctx);

	EBTNodeResult tickCondition(CBTContext& ctx, const std::string& variable, float dt, bool allow_aborts = true);
	EBTNodeResult navigateToPosition(CBTContext& ctx, float move_speed, float rotate_speed, float acceptance_dist, float max_accept_dist, float dt, bool face_target = true);

	CHandle getPlayer();

	float getAdjustedParameter(TCompGameManager::EParameterDDA param_type, float value);

public:

	float move_speed;

	////////////////////////////////////////////
	// Generic parameters loaded from OWLBT. Each task will use them in for its own purpose.
	float number_field[4] = { 0.f };

	std::string string_field = std::string();
	bool bool_field = false;
	////////////////////////////////////////////

	bool cancellable_on_hit = true;
	bool cancellable_on_decision = true;

	// Called after onEnter and onUpdate task
	virtual EBTNodeResult executeTask(CBTContext& ctx, float dt) { return EBTNodeResult::SUCCEEDED; }
	
	// Called when the task is entered
	virtual void onEnter(CBTContext& ctx) {}
	
	virtual void cleanTask(CBTContext& ctx) {}
	
	virtual void init() {}
	
	std::string name;
	std::string_view type;

};

using CBTTaskDummy = IBTTask;