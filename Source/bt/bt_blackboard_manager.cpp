#include "mcv_platform.h"
#include "bt_blackboard_manager.h"

void CBTBlackboardManager::registerBlackboard(CBTBlackboard* bb) {
	blackboards.push_back(bb);
}