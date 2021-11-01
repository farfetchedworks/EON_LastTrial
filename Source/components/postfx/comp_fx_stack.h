#pragma once
#include "components/common/comp_base.h"
#include "components/messages.h"

class CTexture;

struct TCompFXStack : public TCompBase
{
	DECL_SIBLING_ACCESS();

	bool enabled = true;
	std::vector<std::pair<int, CHandle>> fx_list;

	CHandle last_fx;
	static CTexture* currentRT;
	static CTexture* lastRT;

	void load(const json& j, TEntityParseContext& ctx);
	void debugInMenu();

	CTexture* render(CTexture* rt, CTexture* last_rt);
	void reorder(CHandle fx, bool dir);
	void sort();
	void onNewFx(const TMsgFXToStack& msg);

	static void updateRT(std::function<CTexture*(CTexture*, CTexture*)> fn) {
		currentRT = fn(currentRT, lastRT);
	}
    
	static void registerMsgs() {
        DECL_MSG(TCompFXStack, TMsgFXToStack, onNewFx);
    }
};

