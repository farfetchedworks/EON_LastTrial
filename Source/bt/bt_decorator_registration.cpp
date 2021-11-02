#include "mcv_platform.h"
#include "bt_blackboard.h"
#include "bt_parser.h"
#include "bt_context.h"
#include "bt.h"
#include "components/common/comp_transform.h"
#include "components/controllers/comp_ai_controller_base.h"
#include "components/stats/comp_health.h"
#include "components/controllers/pawn_utils.h"
#include "task_utils.h"
#include "components/gameplay/comp_game_manager.h"
#include "skeleton/comp_skel_lookat.h"

#pragma region Utilities

// Returns true if v1 <cmp_sign> v2. e.g.: v1 > v2, v1 != v2, etc.
template <typename T>
bool compare(const std::string& cmp_sign, const T v1, const T v2)
{
	if (cmp_sign == ">") {
		return v1 > v2;
	}
	else if (cmp_sign == ">=") {
		return v1 >= v2;
	}
	else if (cmp_sign == "<") {
		return v1 < v2;
	}
	else if (cmp_sign == "<=") {
		return v1 <= v2;
	}
	else if (cmp_sign == "!=") {
		return v1 != v2;
	}
	else if (cmp_sign == "==") {
		return v1 == v2;
	}
	else {
		fatal("Unsupported compare sign\n");
		return false;
	}
}


#pragma endregion

/*
*	Declare and implement all decorators here
*	The macros at the bottom will register each of them
*/

#pragma region General Decos

class CBTDecoPlayerInSight : public IBTDecorator
{
public:

	EBTNodeResult evaluateCondition(CBTContext& ctx, float dt)
	{
		TCompAIControllerBase* h_controller = ctx.getComponent<TCompAIControllerBase>();

		CEntity* h_player = TaskUtils::getPlayer();

		TCompSkelLookAt* h_skellookat = ctx.getComponent<TCompSkelLookAt>();

		// Player is in sight when it is inside the enemy's cone and there are no colliders in between them
		
		// If the entity has a look at bone, use it to detect the player
		bool is_inside_cone = (h_skellookat && h_skellookat->head_bone_name != "") ?
			PawnUtils::isInsideConeOfBone(ctx.getOwnerEntity(), h_player, h_skellookat->head_bone_name, h_controller->fov_rad, h_controller->sight_radius) :
			PawnUtils::isInsideCone(ctx.getOwnerEntity(), h_player, h_controller->fov_rad, h_controller->sight_radius);
		bool can_see_player = TaskUtils::canSeePlayer(ctx.getComponent<TCompTransform>(), h_player->get<TCompTransform>());

		return  is_inside_cone && can_see_player ?
			calculateResult(EBTNodeResult::SUCCEEDED) : calculateResult(EBTNodeResult::FAILED);
	}
};

class CBTDecoHealthPercent : public IBTDecorator
{
public:

	EBTNodeResult evaluateCondition(CBTContext& ctx, float dt)
	{
		TCompHealth* h_health = ctx.getComponent<TCompHealth>();

		float max_health = (float)h_health->getMaxHealth();
		float health = (float)h_health->getHealth();
		float health_percent = (health / max_health) * 100;

		return compare(string_field, health_percent, number_field) ?
			calculateResult(EBTNodeResult::SUCCEEDED) : calculateResult(EBTNodeResult::FAILED);
	}
};

// Return succeeded when the time has elapsed. When the time is out, it will return succeed, aborting or returning succeed to its parent
// The children will continue executing until time elapses
class CBTDecoTimer : public IBTDecorator
{
	float max_time = 0.f;

public:
	void init() override {
		// Load generic parameters
		max_time = number_field;
		is_loop_type = true;
		check_periodically = true;
	}

	void onEnter(CBTContext& ctx) override {
		ctx.setNodeVariable(name, "current_time", 0.f);
	}

	EBTNodeResult evaluateCondition(CBTContext& ctx, float dt)
	{
		float curr_time = ctx.getNodeVariable<float>(name, "current_time");
		curr_time += dt;
		ctx.setNodeVariable(name, "current_time", curr_time);

		if (curr_time < max_time)
			return calculateResult(EBTNodeResult::IN_PROGRESS);
		else {
			ctx.setNodeVariable(name, "current_time", 0.f);
			return calculateResult(EBTNodeResult::SUCCEEDED);
		}
	}

};

/*
	The first time it executes it is successful. The second time it will return failed until
	time elapses
*/
class CBTDecoCooldown : public IBTDecorator
{
	float cooldown_time = 0.f;

public:
	void init() override {
		// Load generic parameters
		cooldown_time = getAdjustedParameter(TCompGameManager::EParameterDDA::COOLDOWN, number_field);
		background = true;
	}

	void onEnter(CBTContext& ctx) override {
		// The first time it enters current time must be -1
		if (!ctx.isNodeVariableValid(name, "current_time")) {
			ctx.setNodeVariable(name, "current_time", -1.f);
		}
	}

	// When this node exits it should set itself as background decorator and initialize the time
	void onExit(CBTContext& ctx) override {
		ctx.addBGDecorator(this);
		ctx.setNodeVariable(name, "current_time", cooldown_time);
	}

	// Separate execution from condition evaluation
	void execBackground(CBTContext& ctx, float dt) override 
	{
		// decrement the current time
		float curr_time = ctx.getNodeVariable<float>(name, "current_time");
		curr_time -= dt;

		// When the cooldown has elapsed, set itself to 0 and remove itself as background decorator
		if (curr_time <= 0.f) {
			curr_time = 0.f;
			ctx.removeBGDecorator(this);
		}
		ctx.setNodeVariable(name, "current_time", curr_time);
	}

	EBTNodeResult evaluateCondition(CBTContext& ctx, float dt)
	{
		float curr_time = ctx.getNodeVariable<float>(name, "current_time");

		// If it is the first time, return succeeded
		if (curr_time == -1.f) {
			return EBTNodeResult::SUCCEEDED;
		}

		// If time has not elapsed, return FAILED
		if (curr_time > 0.f) {
			return EBTNodeResult::FAILED;
		}
		// If time has elapsed, initialize the timer, remove itself as background deco and return SUCCEEDED
		else {
			return EBTNodeResult::SUCCEEDED;
		}
	}
};


/*
	The first time it executes it is successful. The second time it will return failed until
	time elapses.
	All decorators accessing the same BB key with the current time will unlock themselves
	when time has expired
	It will always consider the cooldown of the first decorator of this type that it has encountered.
	For example, if A has a cooldown of 5s and B of 3s, it will only return succeeded when 5s have elapsed
	This is because only the first GlobalCooldown will be stored as background
*/
class CBTDecoGlobalCooldown : public IBTDecorator
{
	float cooldown_time = 0.f;
	std::string bbkey_current_time;

public:
	void init() override {
		// Load generic parameters	
		cooldown_time = getAdjustedParameter(TCompGameManager::EParameterDDA::COOLDOWN, number_field);
		bbkey_current_time = string_field;
		background = true;

		if (bbkey_current_time == "") {
			dbg("Global decorator %s must have a BB key for the current time", name);
		}
	}

	void onEnter(CBTContext& ctx) override {
		// The first time it enters current time must be -1
		if (!ctx.getBlackboard()->isValid(bbkey_current_time)) {
			ctx.getBlackboard()->setValue<float>(bbkey_current_time, -1.f);
		}
	}

	// When this node exits it should set itself as background decorator and initialize the time
	void onExit(CBTContext& ctx) override {

		// Only add itself as BG when there is no other one already changing it or if it is the first time entering the node
		float curr_time = ctx.getBlackboard()->getValue<float>(bbkey_current_time);
		if (curr_time == 0.f || curr_time == -1.f) {
			ctx.addBGDecorator(this);
			ctx.getBlackboard()->setValue<float>(bbkey_current_time, cooldown_time);
		}
	}


	// Separate execution from condition evaluation
	void execBackground(CBTContext& ctx, float dt) override {
		// decrement the current time
		float curr_time = ctx.getBlackboard()->getValue<float>(bbkey_current_time);
		curr_time -= dt;

		// When the cooldown has elapsed, set itself to 0 and remove itself as background decorator
		if (curr_time <= 0.f) {
			curr_time = 0.f;
			ctx.removeBGDecorator(this);
		}

		ctx.getBlackboard()->setValue<float>(bbkey_current_time, curr_time);
	}

	EBTNodeResult evaluateCondition(CBTContext& ctx, float dt)
	{
		float curr_time = ctx.getBlackboard()->getValue<float>(bbkey_current_time);

		// If it is the first time, return succeeded
		if (curr_time == -1.f) {
			return EBTNodeResult::SUCCEEDED;
		}

		// If time has not elapsed, return FAILED
		if (curr_time > 0.f) {
			return EBTNodeResult::FAILED;
		}
		// If time has elapsed, initialize the timer, remove itself as background deco and return SUCCEEDED
		else {
			return EBTNodeResult::SUCCEEDED;
		}
	}
};

#pragma endregion

#pragma region Flow Decos

class CBTDecoLoop : public IBTDecorator
{
private:
	int max_iterations = 1;

public:

	void init() override {
		// Load generic parameters
		is_loop_type = true;
		max_iterations = (int)number_field;
	}

	void onEnter(CBTContext& ctx) override {
		// dbg("enter");
	}

	EBTNodeResult evaluateCondition(CBTContext& ctx, float dt)
	{
		int iteration = ctx.getNodeVariable<int>(name, "current_iteration");
		iteration++;

		if (iteration <= max_iterations) {
			ctx.setNodeVariable(name, "current_iteration", iteration);
			//dbg("Loop %i from %i\n", iteration, max_iterations);
			return calculateResult(EBTNodeResult::IN_PROGRESS);
		}
		else {
			ctx.setNodeVariable(name, "current_iteration", 0);
			return calculateResult(EBTNodeResult::SUCCEEDED);
		}
	}
};

class CBTDecoRandomLoop : public IBTDecorator
{
private:
	int min_iterations = 0;
	int max_iterations = 1;

public:

	void init() override {
		// Load generic parameters
		is_loop_type = true;
		min_iterations = (int)number_field;
		max_iterations = (int)number_field_2;
	}

	void onEnter(CBTContext& ctx) override {
		int iteration = ctx.getNodeVariable<int>(name, "current_iteration");

		// If node is beginning, generate random number of iterations
		if (iteration == 0) {
			int target_iterations = (int)Random::range((float)min_iterations, (float)max_iterations + 1);
			ctx.setNodeVariable(name, "target_iteration", target_iterations);
		}
	}

	EBTNodeResult evaluateCondition(CBTContext& ctx, float dt)
	{
		// Get current iterator
		int iteration = ctx.getNodeVariable<int>(name, "current_iteration");
		iteration++;

		// Get target iteration count
		int target_iteration = ctx.getNodeVariable<int>(name, "target_iteration");

		if (iteration <= target_iteration) {
			ctx.setNodeVariable(name, "current_iteration", iteration);
			// dbg("Loop %i from %i\n", iteration, target_iteration);
			return calculateResult(EBTNodeResult::IN_PROGRESS);
		}
		else {
			ctx.setNodeVariable(name, "current_iteration", 0);
			return calculateResult(EBTNodeResult::SUCCEEDED);
		}
	}
};

class CBTDecoRandom : public IBTDecorator
{
private:
	float odds;

public:
	void init() override {
		// Load generic parameters
		odds = number_field / 100.f;
	}

	EBTNodeResult evaluateCondition(CBTContext& ctx, float dt)
	{
		float dice = Random::range(0.f, 1.f);

		if (dice <= odds)
			return EBTNodeResult::SUCCEEDED;
		else
			return EBTNodeResult::FAILED;
	}
};

#pragma endregion

#pragma region Generic Decos

class CBTDecoCompareBlackboardFloat : public IBTDecorator
{
private:
	std::string key;
	std::string cmp_sign;
	float cmp_num;

public:
	void init() override {
		// Load generic parameters
		key = string_field;
		cmp_sign = string_field_2;
		cmp_num = number_field;
	}

	EBTNodeResult evaluateCondition(CBTContext& ctx, float dt)
	{
		if (!ctx.getBlackboard()->isValid(key)) {
			return calculateResult(EBTNodeResult::FAILED);
		}

		float value = ctx.getBlackboard()->getValue<float>(key);

		return compare(cmp_sign, value, cmp_num) ? calculateResult(EBTNodeResult::SUCCEEDED) : calculateResult(EBTNodeResult::FAILED);
	}
};

class CBTDecoCompareBlackboardInt : public IBTDecorator
{
private:
	std::string key;
	std::string cmp_sign;
	int cmp_num;

public:
	void init() override {
		// Load generic parameters
		key = string_field;
		cmp_sign = string_field_2;
		cmp_num = static_cast<int>(number_field);
	}

	EBTNodeResult evaluateCondition(CBTContext& ctx, float dt)
	{
		if (!ctx.getBlackboard()->isValid(key)) {
			return calculateResult(EBTNodeResult::FAILED);
		}

		int value = ctx.getBlackboard()->getValue<int>(key);

		return compare(cmp_sign, value, cmp_num) ? calculateResult(EBTNodeResult::SUCCEEDED) : calculateResult(EBTNodeResult::FAILED);
	}
};

class CBTDecoIsBlackboardBoolTrue : public IBTDecorator
{
private:
	std::string key;

public:
	void init() override {
		// Load generic parameters
		key = string_field;
	}

	EBTNodeResult evaluateCondition(CBTContext& ctx, float dt)
	{
		if (!ctx.getBlackboard()->isValid(key)) {
			return calculateResult(EBTNodeResult::FAILED);
		}

		bool value = ctx.getBlackboard()->getValue<bool>(key);

		return value ? calculateResult(EBTNodeResult::SUCCEEDED) : calculateResult(EBTNodeResult::FAILED);
	}
};

class CBTDecoIsBlackboardKeyValid : public IBTDecorator
{
private:
	std::string key;

public:
	void init() override {
		// Load generic parameters
		key = string_field;
	}

	EBTNodeResult evaluateCondition(CBTContext& ctx, float dt)
	{
		if (ctx.getBlackboard()->isValid(key)) 
			return calculateResult(EBTNodeResult::SUCCEEDED);

		return calculateResult(EBTNodeResult::FAILED);
	}
};


#pragma endregion

#pragma region Debugging Decos

class CBTDecoSuccess : public IBTDecorator
{
public:
	CBTDecoSuccess() {}
	EBTNodeResult evaluateCondition(CBTContext& ctx, float dt)
	{
		return calculateResult(EBTNodeResult::SUCCEEDED);
	}
};

class CBTDecoFailure : public IBTDecorator
{
public:
	CBTDecoFailure() {}
	EBTNodeResult evaluateCondition(CBTContext& ctx, float dt)
	{
		return calculateResult(EBTNodeResult::FAILED);
	}
};

#pragma endregion

#define REGISTER_DECO(DECO_NAME)  {#DECO_NAME, new CBTDecoratorFactory<CBTDeco##DECO_NAME>()},
std::map<std::string_view, CBTParser::IBTDecoratorFactory*> CBTParser::decorator_types{
	#include "bt_decorator_registration.h"
};
#undef REGISTER_DECO