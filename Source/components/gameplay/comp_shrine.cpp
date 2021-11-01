#include "mcv_platform.h"
#include "render/draw_primitives.h"
#include "comp_shrine.h"
#include "components/common/comp_transform.h"
#include "components/stats/comp_health.h"
#include "components/stats/comp_warp_energy.h"
#include "components/stats/comp_stamina.h"
#include "components/gameplay/comp_game_manager.h"
#include "skeleton/comp_skeleton.h"

DECL_OBJ_MANAGER("shrine", TCompShrine)

void TCompShrine::load(const json& j, TEntityParseContext& ctx)
{
	shrine_info = j.value("info", shrine_info);
	active = j.value("active", active);
}

void TCompShrine::onEntityCreated()
{
	if (!GameManager)
		return;

	// send msg to Game Manager to register me
	// other option is use GM->addShrine() here...
	// ...

	TMsgShrineCreated msg;
	msg.h_shrine = this;
	GameManager->sendMsg(msg);
}

void TCompShrine::onPray(const TMsgShrinePray& msg)
{
	TCompSkeleton* c_skel = get<TCompSkeleton>();
	assert(c_skel);
	
	if (!active)
	{
		active = true;
		pray_pos.player = msg.position;
		pray_pos.collider = msg.collider_pos;
		c_skel->playAnimation("Shrine_active", ANIMTYPE::CYCLE);
	}

	c_skel->playAnimation("Shrine_charge", ANIMTYPE::ACTION);
	
	// Shrine_charge has an associated callback,
	// so the respawn is managed in animation/callbacks/shrine_callback.cpp
}

void TCompShrine::restorePlayer()
{
	CEntity* player = getEntityByName("player");
	if (!player)
	return;

	TMsgRestoreAll msg;
	player->sendMsg(msg);
}

void TCompShrine::debugInMenu()
{
	ImGui::Separator();
	ImGui::Text("Info", shrine_info.c_str());
	ImGui::SameLine();
	ImGui::Checkbox("Active", &active);
	ImGui::Separator();
	CEntity* e = CHandle(this).getOwner();
	e->debugInMenu();
}

void TCompShrine::onEnemyEnter(const TMsgEnemyEnter& msg)
{
	num_enemies++;
}

void TCompShrine::onEnemyExit(const TMsgEnemyExit& msg)
{
	num_enemies--;
}

void TCompShrine::renderDebug()
{
//#ifdef _DEBUG
//	CEntity* player = getEntityByName("player");
//	TCompTransform* t_shrine = get<TCompTransform>();
//	TCompTransform* t_player = player->get<TCompTransform>();
//
//	VEC3 shrine_pos = t_shrine->getPosition();
//	VEC3 player_pos = t_player->getPosition();
//
//	float sq_dist = VEC3::DistanceSquared(shrine_pos, player_pos);
//
//	if (sq_dist < 3.f && canPray()) {
//		drawText2D(VEC2(0, 20), Colors::White, "Pray in shrine [E - PadA]", true);
//	}
//#endif // _DEBUG
}