#include "mcv_platform.h"
#include "engine.h"
#include "comp_npc.h"
#include "modules/module_physics.h"
#include "modules/game/module_player_interaction.h"
#include "input/input_module.h"
#include "components/common/comp_transform.h"
#include "components/controllers/comp_player_controller.h"
#include "components/controllers/pawn_utils.h"
#include "components/common/comp_collider.h"
#include "components/abilities/comp_time_reversal.h"
#include "components/messages.h"
#include "utils/resource_json.h"
#include "render/draw_primitives.h"
#include "modules/module_subtitles.h"

DECL_OBJ_MANAGER("npc", TCompNPC)

void TCompNPC::load(const json& j, TEntityParseContext& ctx)
{
	
}

void TCompNPC::update(float dt)
{
	
}

bool TCompNPC::resolve()
{
	CEntity* player = getEntityByName("player");
	if (!player)
		return false;
	return PawnUtils::isInsideCone(player, CHandle(this).getOwner(), deg2rad(60.f), 2.f);
}

void TCompNPC::interact()
{
	Subtitles.startCaption("npc_melee", getEntity());	
	PlayerInput.blockInput();
}

void TCompNPC::onStop(const TMsgStopCaption& msg)
{
	PlayerInput.unBlockInput();
}