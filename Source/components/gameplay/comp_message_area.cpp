#include "mcv_platform.h"
#include "render/draw_primitives.h"
#include "components/gameplay/comp_message_area.h"
#include "engine.h"
#include "components/gameplay/comp_game_manager.h"
#include "ui/ui_module.h"
#include "ui/widgets/ui_text.h"
#include "engine.h"
#include "input/input_module.h"

DECL_OBJ_MANAGER("message_area", TCompMessageArea)

void TCompMessageArea::load(const json& j, TEntityParseContext& ctx)
{
	message_id = j["message_id"];
	one_time_activation = j.value("one_time_activation", one_time_activation);
	duration = j.value("duration", duration);
	pause_game = j.value("pause_game", pause_game);
}

void TCompMessageArea::onEntityCreated()
{

}

void TCompMessageArea::update(float dt)
{

	if (is_active && PlayerInput["interact"].getsPressed()) {
		clearMessage();
	}

	//if (duration > 0) {
	//	// The message is activated if it is set as active and has not one time activation, or if it is a one-time-message, it has not been already activated
	//	if (is_active && (one_time_activation && !has_been_activated || !one_time_activation)	)					
	//	{
	//		current_time += dt;
	//		showMessage();
	//		if (current_time>=duration) {
	//			is_active = false;
	//		}
	//	}
	//}
	//else {
	//	if (is_active) {
	//		showMessage();
	//	}
	//}

}


void TCompMessageArea::showMessage() 
{
	ui::CText* w_txt_message = (ui::CText*) EngineUI.getWidget("txt_message");

	is_active = true;
	CEntity* e_game_mgr = getEntityByName("game_manager");
	TCompGameManager* c_game_mgr = e_game_mgr->get<TCompGameManager>();

	if (pause_game) {
		c_game_mgr->setTimeStatusLerped(TCompGameManager::ETimeStatus::PAUSE, 0.3f);
	}

	w_txt_message->textParams.text = c_game_mgr->ui_messages[message_id];
	EngineUI.activateWidget("subvert_message");
}

void TCompMessageArea::clearMessage()
{
	if (!is_active) {
		return;
	}
	
	if (EngineUI.getWidget("subvert_message")->isActive()) {
		EngineUI.deactivateWidget("subvert_message");
	}

	is_active = false;

	if (pause_game) {
		CEntity* e_game_mgr = getEntityByName("game_manager");
		TCompGameManager* c_game_mgr = e_game_mgr->get<TCompGameManager>();
		c_game_mgr->setTimeStatusLerped(TCompGameManager::ETimeStatus::NORMAL, 0.3f);
	}
}

void TCompMessageArea::onActivateMsg(const TMsgActivateMsgArea& msg)
{
	if (is_active) {
		return;
	}

	showMessage();
}

void TCompMessageArea::onDeactivateMsg(const TMsgDeactivateMsgArea& msg)
{
	//if (duration <= 0) {
		clearMessage();
	//}
}

void TCompMessageArea::debugInMenu()
{

}