#pragma once

#include "bt_common.h"

class CBTContext;

class IBTDecorator
{

	friend class CBTParser;

private:

protected:	
	EBTObserverAborts obs_abort_type = EBTObserverAborts::NONE;
	bool inverse_check_condition = false;													// if set to true, it will invert the result of the evaluateCondition method
	bool check_periodically = false;															// if set to true, it will be checked every frame by the context
	bool background = false;																			// if set to true, it will keep executing in the background when required by the node
	bool is_loop_type = false;																	// set to true for decorators which loop and return success when they abort or finish (such as timer and loop)

	// Returns the same result if inverse check condition is false, otherwise it inverts the opposite result
	EBTNodeResult calculateResult(EBTNodeResult decoResult) {
		if (!inverse_check_condition)
			return decoResult;

		if (decoResult == EBTNodeResult::FAILED) return EBTNodeResult::SUCCEEDED;
		else if (decoResult == EBTNodeResult::SUCCEEDED) return EBTNodeResult::FAILED;
		else if (decoResult == EBTNodeResult::IN_PROGRESS) return EBTNodeResult::IN_PROGRESS;
		else return EBTNodeResult::ABORTED;
	}

	float getAdjustedParameter(TCompGameManager::EParameterDDA param_type, float value)
	{
		CEntity* e_game_manager = getEntityByName("game_manager");
		TCompGameManager* c_game_manager = e_game_manager->get<TCompGameManager>();

		return c_game_manager->getAdjustedParameter(param_type, value);
	}

public:

	////////////////////////////////////////////
	// Generic parameters loaded from OWLBT. Each decorator will use them in for its own purpose.
	float number_field = 0.f;
	float number_field_2 = 0.f;
	std::string string_field = std::string();
	std::string string_field_2 = std::string();
	////////////////////////////////////////////


	virtual EBTNodeResult evaluateCondition(CBTContext& ctx, float dt) { return EBTNodeResult::SUCCEEDED; }
	virtual void execBackground(CBTContext& ctx, float dt) {}
	virtual void init() {}

	// Called when the decorator is entered
	virtual void onEnter(CBTContext& ctx) {}

	virtual void onExit(CBTContext& ctx) {}

	std::string_view type;

	EBTObserverAborts	getObserverAborts() { return obs_abort_type; }
	bool isCheckedPeriodically() { return check_periodically; }
	bool isLoopType() { return is_loop_type; }
	bool isBackground(){ return background; }

	std::string name;

};

using CBTDecoratorDummy = IBTDecorator;