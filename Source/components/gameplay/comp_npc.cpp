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
	sight_radius = j.value("sight_radius", sight_radius);
	caption_scene = j.value("caption_scene", std::string());
}

void TCompNPC::update(float dt)
{
	
}

bool TCompNPC::resolve()
{
	CEntity* player = getEntityByName("player");
	if (!player)
		return false;
	return PawnUtils::isInsideCone(player, CHandle(this).getOwner(), deg2rad(60.f), sight_radius);
}

void TCompNPC::interact()
{
	if (caption_scene.length())
	{
		Subtitles.startCaption(caption_scene, getEntity());
		PlayerInput.blockInput();
	}
}

void TCompNPC::onStop(const TMsgStopCaption& msg)
{
	PlayerInteraction.setActive(false);
	PlayerInput.unBlockInput();
}