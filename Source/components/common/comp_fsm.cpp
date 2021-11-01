#include "mcv_platform.h"
#include "engine.h"
#include "comp_transform.h"
#include "comp_fsm.h"
#include "fsm/fsm.h"
#include "components/messages.h"
#include "skeleton/comp_skeleton.h"
#include "skeleton/game_core_skeleton.h"
#include "components/controllers/comp_pawn_controller.h"
#include "render/draw_primitives.h"
#include "modules/module_boot.h"

DECL_OBJ_MANAGER("fsm", TCompFSM)

const CGameCoreSkeleton* TCompFSM::_currentCoreSkeleton = nullptr;

void TCompFSM::registerMsgs()
{
	DECL_MSG(TCompFSM, TMsgFSMVariable, onSetVariable);
}

void TCompFSM::load(const json& j, TEntityParseContext& ctx)
{
	assert(j.count("fsm") && j["fsm"].is_string());
	const std::string& filename = j["fsm"];

	TCompSkeleton* c_skel = get<TCompSkeleton>();
	_currentCoreSkeleton = c_skel->getGameCore();
	_context.setFSM(Resources.get(filename)->as<fsm::CFSM>());
	_currentCoreSkeleton = nullptr;

	const std::string& name = j.value("name", "Unnamed FSM");
	_name = name;

	bool enabled = j.value("enabled", true);
	_context.setEnabled(enabled && !Boot.isPreloading());
}

void TCompFSM::onEntityCreated()
{
	_context.setOwnerEntity(CHandle(this).getOwner());
	assert(_context.isValid());

	_context.start();
}

void TCompFSM::update(float dt)
{
	PROFILE_FUNCTION("update " + _name);

	TCompPawnController* controller = get<TCompPawnController>();

	if (controller)
	{
		dt *= controller->speed_multiplier;
	}

	_context.update(dt);
}

void TCompFSM::debugInMenu()
{
	_context.renderInMenu();
}

void TCompFSM::onSetVariable(const TMsgFSMVariable& msg)
{
	_context.setVariableValue(msg.name, msg.value);
}

void TCompFSM::renderDebug()
{
#ifdef _DEBUG
	TCompTransform* c_transform = get<TCompTransform>();
	VEC3 pos = c_transform->getPosition();
	pos.y += 2.f;
	drawText3D(pos, Colors::White, getCurrentState()->name.c_str());
#endif // _DEBUG
}

const fsm::IState* TCompFSM::getCurrentState()
{
	return _context.getCurrentState();
}
