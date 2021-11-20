#include "mcv_platform.h"
#include "entity/entity.h"
#include "engine.h"
#include "animation/animation_callback.h"
#include "components/cameras/comp_camera_shake.h"
#include "components/gameplay/comp_game_manager.h"
#include "components/controllers/pawn_utils.h"
#include "components/stats/comp_health.h"
#include "components/common/comp_parent.h"
#include "skeleton/comp_attached_to_bone.h"
#include "entity/entity_parser.h"
#include "lua/module_scripting.h"
#include "bt/task_utils.h"
#include "ui/ui_module.h"

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
			t.setScale(damp<VEC3>(t.getScale(), VEC3(0.8f), 6.f, Time.delta));
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

		TCompParent* parent = e_owner->get<TCompParent>();
		CEntity* hole = parent->getChildByName("Cygnus_black_hole");
		VEC3 hole_pos = hole->getPosition();
		PawnUtils::spawnGeons(hole_pos, e_owner);

		CTransform t;
		t.setPosition(hole_pos);
		spawn("data/prefabs/eter.json", t);
	}
};

REGISTER_CALLBACK("onCygnusDeathCallback", onCygnusDeathCallback)