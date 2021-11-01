#include "mcv_platform.h"
#include "trigger_area.h"
#include "components/gameplay/comp_trigger_area.h"

CHandle ITriggerArea::getAreaTrigger(CHandle event_trigger)
{
	CEntity* e_trigger_area = event_trigger;
	TCompTriggerArea* c_trigger_area = e_trigger_area->get<TCompTriggerArea>();
	return c_trigger_area->h_hit_object;
}