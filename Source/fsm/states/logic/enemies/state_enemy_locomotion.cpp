#include "mcv_platform.h"
#include "fsm/fsm_parser.h"
#include "components/ai/comp_bt.h"
#include "components/common/comp_transform.h"
#include "components/common/comp_collider.h"
#include "components/controllers/comp_ai_controller_base.h"
#include "bt/task_utils.h"
#include "fsm/states/logic/state_locomotion.h"

using namespace fsm;

class CStateEnemyLocomotion : public CStateLocomotion
{
	VEC2 getBlendSpeed(CContext& ctx) const
	{
		CEntity* owner = ctx.getOwnerEntity();
		TCompCollider* collider = owner->get<TCompCollider>();
		assert(collider);
		if (collider->getLinearVelocity().Length() < 0.15f)
			return VEC2::Zero;

		float current_speed = 0.f;
		VEC3 move_dir;

		// Retrieve move speed from current task
		TCompBT* my_bt = owner->get<TCompBT>();
		if (my_bt) {
			current_speed = my_bt->getMoveSpeed();
			move_dir = my_bt->getMoveDirection();
		}

		if(current_speed < 0.15f)
			return VEC2::Zero;

		move_dir.Normalize();

		TCompTransform* transform = owner->get<TCompTransform>();
		float dx = move_dir.Dot(transform->getRight());
		float dz = move_dir.Dot(transform->getForward());

		return VEC2(dx, dz) * current_speed;
	}

public:
    void onUpdate(CContext& ctx, float dt) const
    {
        CEntity* owner = ctx.getOwnerEntity();
		ctx.setVariableValue("blend_factor", getBlendSpeed(ctx));
        anim.update(ctx, dt);
    }
};

REGISTER_STATE("enemy_locomotion", CStateEnemyLocomotion)