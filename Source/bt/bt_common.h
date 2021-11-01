#pragma once

#include "mcv_platform.h"

enum class EBTNodeType {
	SEQUENCE = 0,
	SELECTOR,
	PARALLEL,
	TASK,
	RANDOM,
	DECORATOR,
	SERVICE
};

enum class EBTNodeResult {
	FAILED = 0,
	SUCCEEDED,
	ABORTED,
	IN_PROGRESS
};

/*
	Enum with the behavior of the decorator as an observer when there is a change in the blackboard
	- None: the decorator will not observe any value in the blackboard
	
	The following cases will abort the execution (returnung "abort" to its parent) if there is a change in a variable which changes the decorator condition
	- Self: aborts only its subtree.
		When its subtree ends, it removes itself from the observers vector

	- Lower priority: aborts the siblings to the right (if any).
		When its subtree ends, it remains in the observers vector, until its parent has to return (the parent removes its observers when it finishes)

	- Both: aborts its subtree and the siblings to the right
*/

enum class EBTObserverAborts {
	NONE = 0,
	SELF,
	LOWER_PRIORITY,
	BOTH
};

using TBTBlackboardKey = std::variant<int, bool, float, std::string, CHandle, VEC3, QUAT>;

/*
	Struct with the entries of the blackboard, containing:
	- is_set: if true, the value has been set during execution. If the value is cleared or has not been initialized, this is false
	- instance_synced: if true, the value of this key will be shared between all of the owners of the behavior tree
	- key_type: type of the entry according to the enum BBType
	- key_value: holds the actual value of the key entry
*/
struct SBBKeyEntry {
	bool is_set = false;
	bool instance_synced = false;
	TBTBlackboardKey key_value;
};

using MBTBlackboardKeys = std::map<std::string, SBBKeyEntry>;
