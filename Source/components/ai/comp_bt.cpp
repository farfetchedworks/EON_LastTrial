#include "mcv_platform.h"
#include "comp_bt.h"
#include "bt/bt.h"
#include "bt/bt_blackboard.h"
#include "components/common/comp_transform.h"
#include "components/common/comp_fsm.h"
#include "fsm/states/logic/state_logic.h"
#include "components/controllers/comp_pawn_controller.h"
#include "components/controllers/comp_ai_controller_base.h"
#include "components/common/comp_collider.h"
#include "render/draw_primitives.h"
#include "components/combat/comp_player_weapon.h"

DECL_OBJ_MANAGER("behavior_tree", TCompBT)

bool TCompBT::UPDATE_ENABLED = true;

void TCompBT::load(const json& j, TEntityParseContext& ctx)
{
	const std::string& filename = j["behavior_tree"];
	bt_context = new CBTContext();
	bt_context->setBT(Resources.get(filename)->as<CBTree>());

	// set the tree as enabled or disabled
	const bool enabled = j.value("enabled", true);
	bt_context->setEnabled(enabled);

	// clone the Blackboard of the BT resource to have its own instance for the current entity
	CBTBlackboard* blackboard_resource = bt_context->getTree()->getBlackboard();
	bt_context->setBlackboard(new CBTBlackboard(*blackboard_resource));
	bt_context->getBlackboard()->setContext(bt_context);
}

void TCompBT::onEntityCreated()
{
	CEntity* e_player = getEntityByName("Espasa");
	if (!e_player)
		return;

	// Set the max damage for strong hitstun getting the damage from the player weapon
	TCompPlayerWeapon* c_player_weapon = e_player->get<TCompPlayerWeapon>();

	// The strong damage is stored in the damages vector in the fourth position
	MAX_REGULAR_DMG = c_player_weapon->getDamageByHit(3);

	TCompAIControllerBase* c_cont_base = get<TCompAIControllerBase>();
	c_cont_base->initProperties();

	h_collider = get<TCompCollider>();

	bt_context->setOwnerEntity(CHandle(this).getOwner());
	bt_context->init();
	bt_context->start();	
}

void TCompBT::debugInMenu()
{
	bt_context->renderInMenu();
}

void TCompBT::update(float dt)
{
	if (!UPDATE_ENABLED)
		return;

	TCompPawnController* controller = get<TCompPawnController>();

	if (controller)
	{
		dt *= controller->speed_multiplier;
	}

	bt_context->update(dt);
}

void TCompBT::reload()
{
	bt_context->reset();

	// clone the Blackboard of the BT resource to have its own instance for the current entity
	CBTBlackboard* blackboard_resource = bt_context->getTree()->getBlackboard();
	bt_context->setBlackboard(new CBTBlackboard(*blackboard_resource));
	bt_context->getBlackboard()->setContext(bt_context);
	
	bt_context->init();
	bt_context->start();
}

float TCompBT::getMoveSpeed()
{
	if (!getContext() || !getContext()->getCurrentNode())
		return 0.f;
	else {
		return getContext()->getCurrentNode()->getNodeTask()->move_speed;
	}
}

VEC3 TCompBT::getMoveDirection()
{
	TCompTransform* c_transform = get<TCompTransform>();

	if (!getContext() || !getContext()->getCurrentNode())
		return c_transform->getForward();
	else {
		TCompAIControllerBase* my_cont = get<TCompAIControllerBase>();
		assert(my_cont);
		return my_cont->getMoveDir();
	}
}

void TCompBT::renderDebug()
{
#ifdef _DEBUG

	TCompTransform* c_transform = get<TCompTransform>();
	VEC3 pos = c_transform->getPosition();

	// Debug frames
	{
		TCompFSM* fsm = get<TCompFSM>();
		fsm::CStateBaseLogic* currState = (fsm::CStateBaseLogic*)fsm->getCurrentState();
		if (currState)
		{
			std::string frames = "";
			if (currState->inStartupFrames(fsm->getCtx())) frames = "STARTUP";
			else if (currState->inActiveFrames(fsm->getCtx())) frames = "ACTIVE";
			else if (currState->inRecoveryFrames(fsm->getCtx())) frames = "RECOVERY";
			drawText3D(pos + VEC3(0, 2.5, 0), Colors::Red, frames.c_str(), 40.f);
		}
	}

	pos.y += 3.f;

	CBTNode* current_node = bt_context->getCurrentNode();
	CBTNode* current_second_node = bt_context->getCurrentNode(true);

	if (current_node) {
		drawText3D(pos, Colors::Blue, current_node->getName().c_str());
	}

	pos.y -= 0.5f;
	if (current_second_node) {
		drawText3D(pos, Colors::Green, current_second_node->getName().c_str());
	}
#endif // _DEBUG
}

// MESSAGE CALLBACKS
void TCompBT::onHit(const TMsgHit& msg)
{
	// If hes already dying do nothing
	if (bt_context->isDying())
		return;

	// If current task hasnt been set or is cancellable on hit, set pending processHit
	if (bt_context->canBeCancelledOnHit()) {

		TCompFSM* c_fsm = get<TCompFSM>();
		fsm::CContext anim_ctx = c_fsm->getCtx();
		const fsm::IState* state = c_fsm->getCtx().getCurrentState();
		const fsm::CStateBaseLogic* cast_state = static_cast<const fsm::CStateBaseLogic*>(state);

		// Uncomment to restrict aborts only during active frames
		//if (!(cast_state->usesTimings() && cast_state->inActiveFrames(anim_ctx))) {
				
			// Perform a strong damage animation if the damage exceeds a maximum
			if (msg.damage >= MAX_REGULAR_DMG) {
				bt_context->getBlackboard()->setValue("strongDamage", true);
			}

			int cur_hits = bt_context->getBlackboard()->getValue<int>("processHit");
			bt_context->getBlackboard()->setValue("processHit", ++cur_hits);

			if (msg.fromBack) {
				if(bt_context->getBlackboard()->hasKey("isHitstunBack"))
					bt_context->getBlackboard()->setValue("isHitstunBack", true);
			}
		}
	//}

	// Always reduce health
	TMsgReduceHealth hmsg;
	hmsg.damage = msg.damage;
	hmsg.h_striker = msg.h_striker;
	hmsg.hitByPlayer = msg.hitByPlayer;
	CHandle(this).getOwner().sendMsg(hmsg);
}

void TCompBT::onPreHit(const TMsgPreHit& msg)
{
	// If hes already dying do nothing
	if (bt_context->isDying())
		return;

	// If current task hasnt been set or is cancellable on decision, set pending processPreHit
	if (bt_context->canBeCancelledOnDecision()) {
		bt_context->getBlackboard()->setValue("processPreHit", true);
	}
}

void TCompBT::onParry(const TMsgEnemyStunned& msg)
{
	// If hes already dying do nothing
	if (bt_context->isDying())
		return;

	// Must forcefully (always) process the sucessful parry
	bt_context->getBlackboard()->setValue("processParry", true);
}

void TCompBT::onEonIsDead(bool is_eon_dead)
{
	bt_context->getBlackboard()->setValue<bool>("isEonDead", is_eon_dead);
}