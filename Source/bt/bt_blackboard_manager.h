#pragma once

#include "mcv_platform.h"
#include "bt_blackboard.h"

class CBTBlackboardManager
{

private:
	CBTBlackboardManager();

	static CBTBlackboardManager* singleton;
	std::vector<CBTBlackboard*> blackboards;

public:
	static CBTBlackboardManager* getInstance()
	{
		if (singleton == nullptr) {
			singleton = new CBTBlackboardManager();
		}

		return singleton;
	};

	void registerBlackboard(CBTBlackboard*);
	void updateSyncVariable();

};