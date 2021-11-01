#include "mcv_platform.h"
#include "engine.h"
#include "entity/entity.h"
#include "modules/module_events.h"
#include "components/gameplay/comp_trigger_area.h"
#include "components/messages.h"
#include "components/gameplay/comp_game_manager.h"

DECL_OBJ_MANAGER("trigger_area", TCompTriggerArea)

void TCompTriggerArea::load(const json& j, TEntityParseContext& ctx)
{
	enabled = j.value("enabled", enabled);
	restrict_eon = j.value("restrict_eon", restrict_eon);
	area_name = j.value("area_name", area_name);

	remove_after_enter = j.value("remove_after_enter", remove_after_enter);
	remove_after_exit = j.value("remove_after_exit", remove_after_exit);
}

void TCompTriggerArea::onEntityCreated()
{

}

/*
	Messages callbacks
*/

void TCompTriggerArea::onTriggerEnter(const TMsgEntityTriggerEnter& msg)
{
	CEntity* e_object_hit = msg.h_entity;
	CEntity* e_player = getEntityByName("player");

	// Store the hit object handle to be used in trigger area classes
	h_hit_object = CHandle(e_object_hit);

	if (restrict_eon && e_object_hit != e_player)
		return;

	// Dispatch the event with the trigger area name and the entity that holds the trigger area comp
	std::string event_name = "TriggerEnter@" + area_name;
	EventSystem.dispatchEvent(event_name, CHandle(this).getOwner());

	if (remove_after_enter)
	{
		destroyTrigger();
	}
}

void TCompTriggerArea::onTriggerExit(const TMsgEntityTriggerExit& msg)
{
	CEntity* e_object_hit = msg.h_entity;
	CEntity* e_player = getEntityByName("player");

	// Store the hit object handle to be used in trigger area classes
	h_hit_object = CHandle(e_object_hit);

	if (restrict_eon && e_object_hit != e_player)
		return;

	// Dispatch the event with the trigger area name and the entity that holds the trigger area comp
	std::string event_name = "TriggerExit@" + area_name;
	EventSystem.dispatchEvent(event_name, CHandle(this).getOwner());

	if (remove_after_exit)
	{
		destroyTrigger();
	}
}

void TCompTriggerArea::destroyTrigger()
{
	// If this is done, for example, after entering an area message, then the AreaMessage event will be removed from the eventsystem
	// But if another message appears, there will be an assert in the event manager because the event is not found

	//CEntity* e_game_manager = getEntityByName("game_manager");
	//e_game_manager->sendMsg(TMsgUnregEvent({ "TriggerEnter@" + area_name }));
	//e_game_manager->sendMsg(TMsgUnregEvent({ "TriggerExit@" + area_name }));

	CHandle(this).getOwner().destroy();
}

void TCompTriggerArea::renderDebug()
{

}
