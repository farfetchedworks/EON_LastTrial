#include "mcv_platform.h"
#include "engine.h"
#include "comp_npc.h"
#include "components/messages.h"
#include "modules/module_physics.h"
#include "modules/game/module_player_interaction.h"
#include "components/common/comp_transform.h"
#include "components/controllers/comp_player_controller.h"
#include "components/controllers/pawn_utils.h"
#include "components/common/comp_collider.h"
#include "components/abilities/comp_time_reversal.h"
#include "components/audio/comp_audio_emitter.h"
#include "input/input_module.h"
#include "skeleton/comp_skel_lookat.h"
#include "utils/resource_json.h"
#include "render/draw_primitives.h"
#include "modules/module_subtitles.h"

DECL_OBJ_MANAGER("npc", TCompNPC)

void TCompNPC::load(const json& j, TEntityParseContext& ctx)
{
	assert(j.count("caption_scenes"));
	sight_radius = j.value("sight_radius", sight_radius);
	talk_3d = j.value("talk_3d", talk_3d);
	
	unique_caption_scene = j["caption_scenes"].value("unique", std::string());
	caption_scene = j["caption_scenes"].value("the_rest", std::string());

	assert(unique_caption_scene.length());
}

void TCompNPC::update(float dt)
{
	
}

bool TCompNPC::resolve()
{
	CEntity* player = getEntityByName("player");
	if (!player)
		return false;
	return PawnUtils::isInsideCone(player, CHandle(this).getOwner(), deg2rad(80.f), sight_radius);
}

void TCompNPC::interact()
{
	// TODO Isaac:
	// Parar el emitter del NPC

	CEntity* trigger = talk_3d ? getEntity() : nullptr;

	if (!first_interaction)
	{
		Subtitles.startCaption(unique_caption_scene, trigger);
		PlayerInput.blockInput();
		first_interaction = true;

	}else if(caption_scene.length())
	{
		Subtitles.startCaption(caption_scene, trigger);
	}

	// Enable player look at
	CEntity* player = getEntityByName("player");
	TCompSkelLookAt * look_at = player->get<TCompSkelLookAt>();
	look_at->setEnabled(true);
	look_at->setTarget(getEntity()->getPosition());
}

void TCompNPC::onStop(const TMsgStopCaption& msg)
{
	PlayerInteraction.setActive(false);
	PlayerInput.unBlockInput();

	// Disable look at
	CEntity* player = getEntityByName("player");
	TCompSkelLookAt* look_at = player->get<TCompSkelLookAt>();
	look_at->stopLooking();
}