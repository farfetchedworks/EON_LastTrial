#pragma once
#include "entity/entity.h"
#include "components/common/comp_base.h"
#include "components/messages.h"

enum class EWidgetType {
	TEXT,
	IMAGE
};

class TCompMessageArea : public TCompBase {

	DECL_SIBLING_ACCESS();

private:

	bool one_time_activation	= false;    //It can be activated only once, then it will disappear
	bool pause_game				= false;    // The game will be paused until the player interacts
	bool closable				= false;    // Closes on interact
	bool activated				= false;
	bool is_active				= false;
	bool inside					= true;

	std::string message_id		= "";					// Message to display
	std::string widget_name		= "";					// Widget to display
	EWidgetType widget_type		= EWidgetType::TEXT;    // Widget Type

	float current_time			= 0.f;
	float duration				= 2.0;      // Time during which the message will appear. If duration is 0, the message is permanent and disappears when the player exits the trigger

	void showMessage();
	void clearMessage();

public:

	void load(const json& j, TEntityParseContext& ctx);
	void onAllEntitiesCreated(const TMsgAllEntitiesCreated& msg);
	void update(float dt);
	void debugInMenu();

	void onActivateMsg(const TMsgActivateMsgArea& msg);
	void onDeactivateMsg(const TMsgDeactivateMsgArea& msg);

	static void registerMsgs() {
		DECL_MSG(TCompMessageArea, TMsgActivateMsgArea, onActivateMsg);
		DECL_MSG(TCompMessageArea, TMsgDeactivateMsgArea, onDeactivateMsg);
		DECL_MSG(TCompMessageArea, TMsgAllEntitiesCreated, onAllEntitiesCreated);
	}
};
