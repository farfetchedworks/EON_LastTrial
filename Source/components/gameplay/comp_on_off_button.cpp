#include "mcv_platform.h"
#include "handle/handle.h"
#include "engine.h"
#include "components/gameplay/comp_on_off_button.h"
#include "components/common/comp_transform.h"
#include "entity/entity.h"
#include "components/common/comp_collider.h"
#include "components/messages.h"

DECL_OBJ_MANAGER("on_off_button", TCompOnOffButton)

void TCompOnOffButton::load(const json& j, TEntityParseContext& ctx)
{

}

void TCompOnOffButton::debugInMenu()
{

}

void TCompOnOffButton::onEntityCreated()
{

}

void TCompOnOffButton::update(float dt)
{

}

void TCompOnOffButton::setParameters(CHandle target_a, CHandle target_b, bool reset_state)
{
	target_a_to_activate = target_a;
	target_b_to_activate = target_b;
	has_to_be_reset = reset_state;
}

/*
	Messages callbacks
*/

void TCompOnOffButton::onPlayerPress(const TMsgEntityTriggerEnter& msg)
{
	CEntity* e_object_hit = msg.h_entity;
	CEntity* e_player = getEntityByName("player");

	// If the player pressed the button, send a message to the targets to activate them
	if (e_object_hit == e_player) {
		TMsgButtonActivated msgButtonActivated;
		TMsgButtonDeactivated msgButtonDeactivated;
		CEntity* e_target_a = target_a_to_activate;

		// If it is deactivated, activate A and if it applies, deactivate B
		if (is_deactivated) {
			e_target_a->sendMsg(msgButtonActivated);
			if (target_b_to_activate.isValid()) {
				CEntity* e_target_b = target_b_to_activate;
				e_target_b->sendMsg(msgButtonActivated);
			}
			is_deactivated = false;
		} // If it was activated, deactivate A and if it applies, activate B
		else {
			e_target_a->sendMsg(msgButtonDeactivated);
			if (target_b_to_activate.isValid()) {
				CEntity* e_target_b = target_b_to_activate;
				e_target_b->sendMsg(msgButtonDeactivated);
			}
			is_deactivated = true;
		}
	}
}


void TCompOnOffButton::restoreState()
{

	if (has_to_be_reset && !is_deactivated) {
	// If the button's state has to be reset set it on its original state (deactivate everything again)

		//TMsgButtonActivated msgButtonActivated;
		TMsgButtonDeactivated msgButtonDeactivated;
		CEntity* e_target_a = target_a_to_activate;


		e_target_a->sendMsg(msgButtonDeactivated);
		if (target_b_to_activate.isValid()) {
			CEntity* e_target_b = target_b_to_activate;
			e_target_b->sendMsg(msgButtonDeactivated);
		}
		is_deactivated = true;
	}
}

void TCompOnOffButton::renderDebug()
{


}
