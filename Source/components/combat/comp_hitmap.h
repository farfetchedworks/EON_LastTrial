#pragma once
#include "entity/entity.h"

class TCompHitmap : public TCompBase {

	struct TCharacterHitInfo {
		CHandle h;
		bool allowed = true;
	};

	std::map<uint32_t, TCharacterHitInfo> m_canHit;

public:

	void onEntityCreated();
	void debugInMenu();

	bool resolve(CHandle h);
	void reset();
};
