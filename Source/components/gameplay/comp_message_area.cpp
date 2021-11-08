#include "mcv_platform.h"
#include "engine.h"
#include "comp_message_area.h"
#include "components/gameplay/comp_game_manager.h"
#include "ui/ui_module.h"
#include "ui/widgets/ui_text.h"
#include "ui/widgets/ui_image.h"
#include "input/input_module.h"

static NamedValues<EWidgetType>::TEntry widget_types[] = {
  {EWidgetType::TEXT, "text"},
  {EWidgetType::IMAGE, "image"}
};
static NamedValues<EWidgetType> widget_type_names(widget_types, sizeof(widget_types) / sizeof(NamedValues<EWidgetType>::TEntry));

DECL_OBJ_MANAGER("message_area", TCompMessageArea)

void TCompMessageArea::load(const json& j, TEntityParseContext& ctx)
{
	message_id = j["message_id"];
	assert(message_id.length() > 0);
	widget_name = j["widget_name"];
	assert(widget_name.length() > 0);
	widget_type = widget_type_names.valueOf(j.value("widget_type", "image").c_str());
	one_time_activation = j.value("one_time_activation", one_time_activation);
	duration = j.value("duration", duration);
	pause_game = j.value("pause_game", pause_game);
	closable = j.value("closable", closable);
}

void TCompMessageArea::onAllEntitiesCreated(const TMsgAllEntitiesCreated& msg)
{
	
}

void TCompMessageArea::update(float dt)
{
	if (!is_active)
		return;

	if (closable && PlayerInput["interact"].getsPressed()) {
		clearMessage();
		return;
	}

	current_time += dt;

	if (current_time > duration)
	{
		clearMessage();
	}
}

void TCompMessageArea::showMessage() 
{
	if (one_time_activation && activated)
		return;

	TCompGameManager* c_game_mgr = GameManager->get<TCompGameManager>();

	if (widget_type == EWidgetType::TEXT)
	{
		ui::CText* w = static_cast<ui::CText*>(EngineUI.getWidget(widget_name));
		assert(w);
		w->textParams.text = c_game_mgr->ui_messages[message_id];
	}
	else if (widget_type == EWidgetType::IMAGE)
	{
		ui::CImage* w = static_cast<ui::CImage*>(EngineUI.getWidget(widget_name));
		assert(w);
		w->imageParams.texture = Resources.get(c_game_mgr->ui_messages[message_id])->as<CTexture>();
		assert(w->imageParams.texture);
	}

	is_active = activated = true;

	if (pause_game) {
		c_game_mgr->setTimeStatusLerped(TCompGameManager::ETimeStatus::PAUSE, 0.3f);
	}

	EngineUI.activateWidget(widget_name);
}

void TCompMessageArea::clearMessage()
{
	if (!is_active) {
		return;
	}
	
	is_active = false;
	current_time = 0.f;

	if (EngineUI.getWidget(widget_name)->isActive()) {
		EngineUI.deactivateWidget(widget_name);
	}

	if (pause_game) {
		TCompGameManager* c_game_mgr = GameManager->get<TCompGameManager>();
		c_game_mgr->setTimeStatusLerped(TCompGameManager::ETimeStatus::NORMAL, 0.3f);
	}
}

void TCompMessageArea::onActivateMsg(const TMsgActivateMsgArea& msg)
{
	if (is_active || inside) {
		return;
	}

	inside = true;
	showMessage();
}

void TCompMessageArea::onDeactivateMsg(const TMsgDeactivateMsgArea& msg)
{
	inside = false;
}

void TCompMessageArea::debugInMenu()
{
	ImGui::Text("Timer: %f", current_time);
	ImGui::Text("Duration: %f", duration);
	ImGui::Text("Message ID: %s", message_id.c_str());
	ImGui::Text("Widget: %s", widget_name.c_str());
	ImGui::Checkbox("Active", &is_active);
}