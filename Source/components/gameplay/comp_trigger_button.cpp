#include "mcv_platform.h"
#include "handle/handle.h"
#include "engine.h"
#include "components/gameplay/comp_trigger_button.h"
#include "components/common/comp_transform.h"
#include "entity/entity.h"
#include "components/common/comp_collider.h"
#include "components/messages.h"
#include "render/draw_primitives.h"

DECL_OBJ_MANAGER("trigger_button", TCompTriggerButton)

void TCompTriggerButton::load(const json& j, TEntityParseContext& ctx)
{

}

void TCompTriggerButton::debugInMenu()
{

}

void TCompTriggerButton::onEntityCreated()
{

}

void TCompTriggerButton::update(float dt)
{

}

void TCompTriggerButton::setParameters(CHandle target)
{
	target_to_activate = target;
}

/*
	Messages callbacks
*/

void TCompTriggerButton::onPlayerPress(const TMsgEntityTriggerEnter& msg)
{
	CEntity* e_object_hit = msg.h_entity;
	CEntity* e_player = getEntityByName("player");

	// If the player pressed the button, send a message to the target to activate it
	if (e_object_hit == e_player) {
		TMsgButtonActivated msgButtonActivated;
		if (target_to_activate.isValid()) {
			CEntity* e_target = target_to_activate;
			e_target->sendMsg(msgButtonActivated);
		}
		else {
			dbg("The button %s does not have an associated target\n", getEntity()->getName());
		}
	}
}

void TCompTriggerButton::onEonInteracted(const TMsgEonInteracted& msg)
{
	TMsgButtonActivated msgButtonActivated;
	if (target_to_activate.isValid()) {
		CEntity* e_target = target_to_activate;
		e_target->sendMsg(msgButtonActivated);
	}
	else {
		dbg("The button %s does not have an associated target\n", getEntity()->getName());
	}
}

void TCompTriggerButton::renderDebug()
{
#ifdef _DEBUG
	CEntity* player = getEntityByName("player");
	TCompTransform* t_button = get<TCompTransform>();
	TCompTransform* t_player = player->get<TCompTransform>();

	VEC3 button_pos = t_button->getPosition();
	VEC3 player_pos = t_player->getPosition();

	float sq_dist = VEC3::DistanceSquared(button_pos, player_pos);

	if (sq_dist < 12.f) {
		drawText2D(VEC2(0, 20), Colors::White, "Activate switch [E - PadA]", true);
	}
#endif
}
