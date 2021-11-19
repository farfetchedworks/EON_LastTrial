#include "mcv_platform.h"
#include "engine.h"
#include "render/draw_primitives.h"
#include "comp_shrine.h"
#include "modules/module_physics.h"
#include "components/common/comp_transform.h"
#include "components/stats/comp_health.h"
#include "components/stats/comp_warp_energy.h"
#include "components/stats/comp_stamina.h"
#include "components/gameplay/comp_game_manager.h"
#include "skeleton/comp_skeleton.h"
#include "utils/resource_json.h"

DECL_OBJ_MANAGER("shrine", TCompShrine)

void TCompShrine::load(const json& j, TEntityParseContext& ctx)
{
	shrine_info = j.value("info", shrine_info);
	active = j.value("active", active);

	// load info from animation launchers
	const CJson* launchers = Resources.get("data/animations/launchers.json")->as<CJson>();
	const json& jData = launchers->getJson();
	const json& jItem = jData["is_praying"];
	acceptance_dist = jItem.value("acceptance_dist", 0.f);
	offset = loadVEC3(jItem, "offset");
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
	if (!active)
	{
		active = true;
		pray_pos.player = msg.position;
		pray_pos.collider = msg.collider_pos;
	}

	// Shrine_charge has an associated callback,
	// so the respawn is managed in animation/callbacks/shrine_callback.cpp
}

bool TCompShrine::resolve()
{
	CEntity* player = getEntityByName("player");
	if (!player)
		return false;
	
	bool is_ok = canPray();

	TCompTransform* transform = get<TCompTransform>();
	TCompTransform* player_transform = player->get<TCompTransform>();

	VHandles colliders;
	is_ok &= EnginePhysics.raycast(player_transform->getPosition(),
		player_transform->getForward(), 2.f, colliders, CModulePhysics::FilterGroup::Interactable);

	is_ok &= player_transform->getForward().Dot(transform->getForward()) < 0.f;

	VEC3 pos = transform->getPosition() + offset;
	float dist = VEC3::Distance(pos, player_transform->getPosition());
	is_ok &= (dist < acceptance_dist);

	return is_ok;
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
#ifdef _DEBUG
	CEntity* player = getEntityByName("player");
	TCompTransform* t_shrine = get<TCompTransform>();
	TCompTransform* t_player = player->get<TCompTransform>();

	VEC3 shrine_pos = t_shrine->getPosition();
	VEC3 player_pos = t_player->getPosition();

	std::string ss = "ANGLE: " + std::to_string(t_player->getForward().Dot(t_shrine->getForward()));
	drawText2D(VEC2(0, 20), Colors::White, ss.c_str(), true);
#endif // _DEBUG
}