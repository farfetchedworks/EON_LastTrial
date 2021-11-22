#include "mcv_platform.h"
#include "entity/entity.h"
#include "engine.h"
#include "animation/animation_callback.h"
#include "components/gameplay/comp_game_manager.h"
#include "components/controllers/comp_player_controller.h"
#include "components/controllers/pawn_utils.h"
#include "components/stats/comp_health.h"
#include "components/common/comp_parent.h"
#include "skeleton/comp_attached_to_bone.h"
#include "entity/entity_parser.h"
#include "lua/module_scripting.h"
#include "input/input_module.h"
#include "ui/ui_module.h"
#include "ui/ui_widget.h"

struct onCygnusDeathCallback : public CAnimationCallback
{
	void AnimationUpdate(float anim_time, CalModel* model, CalCoreAnimation* animation, void* userData)
	{
		if (anim_time > 6.75f)
		{
			CEntity* e_owner = getOwnerEntity(userData);
			TCompParent* parent = e_owner->get<TCompParent>();
			CEntity* hole = parent->getChildByName("Cygnus_black_hole");
			TCompAttachedToBone* socket = hole->get<TCompAttachedToBone>();
			CTransform& t = socket->getLocalTransform();
			t.setScale(damp<VEC3>(t.getScale(), VEC3(0.6f), 4.f, Time.delta));
		}
	}

	void AnimationComplete(CalModel* model, CalCoreAnimation* animation, void* userData)
	{
		CEntity* e_owner = getOwnerEntity(userData);
		TCompHealth* c_health = e_owner->get<TCompHealth>();
		if (!c_health)
			return;

		// if the same enemy completed its animation before, don't do anything more
		// in other case, this sets completion to true inside
		if (c_health->checkDeathAnimationCompleted())
			return;

		c_health->setRenderActive(false);

		EngineUI.deactivateWidget("eon_hud");

		ui::CWidget* w_hud = EngineUI.getWidgetFrom("eon_hud", "warp_energy_bar");
		assert(w_hud);
		w_hud->setVisible(false);

		w_hud = EngineUI.getWidgetFrom("eon_hud", "warp_energy_bar_1");
		assert(w_hud);
		w_hud->setVisible(false);

		w_hud = EngineUI.getWidgetFrom("eon_hud", "time_reversal_bar");
		assert(w_hud);
		w_hud->setVisible(false);

		TCompParent* parent = e_owner->get<TCompParent>();
		CEntity* hole = parent->getChildByName("Cygnus_black_hole");
		VEC3 hole_pos = hole->getPosition();

		CTransform t;
		t.setPosition(hole_pos);
		spawn("data/prefabs/Eter_Entero.json", t);

		// Manage player cinematics

		PlayerInput.blockInput();

		CEntity* player = getEntityByName("player");
		TCompPlayerController* controller = player->get<TCompPlayerController>();
		controller->moveTo(hole_pos, 1.5f, 2.f, []() {

			CEntity* player = getEntityByName("player");
			TCompPlayerController* controller = player->get<TCompPlayerController>();
			controller->setVariable("is_attacking_heavy", 1);
			controller->enabled = false;
		});
	}
};

REGISTER_CALLBACK("onCygnusDeathCallback", onCygnusDeathCallback)