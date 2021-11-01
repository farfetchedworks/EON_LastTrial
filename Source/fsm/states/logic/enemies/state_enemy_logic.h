#pragma once

#include "fsm/states/logic/state_logic.h"
#include "fsm/fsm_parser.h"

namespace fsm
{
	class CStateEnemyLogic : public CStateBaseLogic {

		std::string control_variable_name;

		void onFirstFrameLogic(CContext& ctx) const;

	public:

		void onLoad(const json& params);
		void onEnter(CContext& ctx, const ITransition* transition) const;
		void onExit(CContext& ctx) const;
		void onUpdate(CContext& ctx, float dt) const;
	};

	REGISTER_STATE("enemy_logic", CStateEnemyLogic)
}
