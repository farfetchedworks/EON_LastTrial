#pragma once

#include "mcv_platform.h"
#include "bt_task.h"
#include "bt_context.h"
#include "components/common/comp_transform.h"
#include "components/controllers/comp_ai_controller_base.h"
#include "task_utils.h"

EBTNodeResult IBTTask::tickCondition(CBTContext& ctx, const std::string& variable, float dt, bool allow_aborts)
{
	int condition = std::get<int>(ctx.getFSMVariable(variable));

	ctx.setActiveAnimVariable(variable);		// store the active animation variable in case it is aborted and has to be set to 0

	bool hitStunnedCompleted = (variable == "is_hitstunned" && condition == 2);

	if (condition == -1) {
		// Clean pending callbacks
		cleanLogic(ctx);

		// If aborts are not allowed, disable aborts in the context
		if (!allow_aborts)	ctx.allowAborts(false);

		ctx.setFSMVariable(variable, 1);
		return EBTNodeResult::IN_PROGRESS;
	}
	else if (condition == 0 || hitStunnedCompleted) {

		callbacks.onRecoveryFinished(ctx, dt);
		ctx.setFSMVariable("pend_recovery_finished", true);

		// If aborts are not allowed, enable aborts in the context because the animation has finished
		if (!allow_aborts) ctx.allowAborts(true);

		ctx.setFSMVariable(variable, -1);
		ctx.setActiveAnimVariable("");					// reset the active animation variable because the animation has finished
		return EBTNodeResult::SUCCEEDED;
	}
	else {

		updateLogic(ctx, dt);

		// Enable or disable aborts during task execution depending on the state of allow aborts
		/*if (!allow_aborts)*/ ctx.allowAborts(allow_aborts);

		return EBTNodeResult::IN_PROGRESS;
	}
}

void IBTTask::updateLogic(CBTContext& ctx, float dt)
{
	bool pend_first_frame = std::get<bool>(ctx.getFSMVariable("pend_first_frame"));
	bool pend_startup = std::get<bool>(ctx.getFSMVariable("pend_startup"));
	bool pend_active = std::get<bool>(ctx.getFSMVariable("pend_active"));
	bool pend_recovery = std::get<bool>(ctx.getFSMVariable("pend_recovery"));

	if (pend_first_frame) {
		callbacks.onFirstFrame(ctx, dt);
		ctx.setFSMVariable("pend_first_frame", false);
	}

	if (pend_startup) {
		callbacks.onStartup(ctx, dt);
		ctx.setFSMVariable("pend_startup", false);
	}
	else if (pend_active) {
		bool pend_startup_finished = std::get<bool>(ctx.getFSMVariable("pend_startup_finished"));
		if (pend_startup_finished) {
			callbacks.onStartupFinished(ctx, dt);
			ctx.setFSMVariable("pend_startup_finished", false);
		}
		else {
			callbacks.onActive(ctx, dt);
			ctx.setFSMVariable("pend_active", false);
		}
	}
	else if (pend_recovery) {
		bool pend_active_finished = std::get<bool>(ctx.getFSMVariable("pend_active_finished"));
		if (pend_active_finished) {
			callbacks.onActiveFinished(ctx, dt);
			ctx.setFSMVariable("pend_active_finished", false);
		}

		callbacks.onRecovery(ctx, dt);
		ctx.setFSMVariable("pend_recovery", false);
	}

	// RecoverFinished is called from task's tickCondition(), otherwise its never executed
}

void IBTTask::cleanLogic(CBTContext& ctx)
{
	ctx.setFSMVariable("pend_first_frame", false);
	ctx.setFSMVariable("pend_startup", false);
	ctx.setFSMVariable("pend_startup_finished", false);
	ctx.setFSMVariable("pend_active", false);
	ctx.setFSMVariable("pend_active_finished", false);
	ctx.setFSMVariable("pend_recovery", false);
	ctx.setFSMVariable("pend_recovery_finished", false);
}

EBTNodeResult IBTTask::navigateToPosition(CBTContext& ctx, float move_speed, float rotate_speed, float acceptance_dist, float max_accept_dist, float dt, bool face_target)
{
	TCompTransform* h_trans = ctx.getComponent<TCompTransform>();

	std::vector<VEC3> cur_path = ctx.getNodeVariable<std::vector<VEC3>>(name, "path");
	int num_positions = (int)cur_path.size();
	int index = ctx.getNodeVariable<int>(name, "target_index");
	int next_index = index + 1;

	// if the index did not reach the end of the path, move to the next position
	if (index < num_positions - 1) {
		// Movement has not reached the final target
		VEC3 target = cur_path[next_index];
		VEC3 origin = h_trans->getPosition();

		TCompAIControllerBase* h_controller = ctx.getComponent<TCompAIControllerBase>();

		float dist = VEC3::Distance(origin, target);

		// Rotate to face target
		if (face_target)
			TaskUtils::rotateToFace(h_trans, target, rotate_speed, dt);

		// Advance forward
		VEC3 dir = target - origin;
		VEC3 move_dir = TaskUtils::moveAnyDir(ctx, dir, move_speed, dt, dist);

		// Update distance with new position
		origin += move_dir;
		origin.y = target.y = 0;
		dist = VEC3::Distance(origin, target);

		if (dist < acceptance_dist) {
			// Go to the next position in the path vector when it reaches the current target
			int target_idx = ctx.getNodeVariable<int>(name, "target_index");
			ctx.setNodeVariable(name, "target_index", ++target_idx);
		}

		return EBTNodeResult::IN_PROGRESS;
	}
	else {
		// Movement has reached target
		return EBTNodeResult::SUCCEEDED;
	}
}

CHandle IBTTask::getPlayer()
{
	return TaskUtils::getPlayer();
}

float IBTTask::getAdjustedParameter(TCompGameManager::EParameterDDA param_type, float value)
{
	CEntity* e_game_manager = getEntityByName("game_manager");
	TCompGameManager* c_game_manager = e_game_manager->get<TCompGameManager>();

	return c_game_manager->getAdjustedParameter(param_type, value);
}