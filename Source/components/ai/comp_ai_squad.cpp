#include "mcv_platform.h"
#include "comp_ai_squad.h"

DECL_OBJ_MANAGER("brain", TCompIASquadBrain)

void TCompIASquadBrain::onPlayerSeen(const TMsgPlayerSeen& msg)
{
	if (!in_alarm)
	{
		in_alarm = true;
		dbg("player seen!");
	}
}

void TCompIASquadBrain::onNewMember(const TMsgRegisterInSquad& msg)
{
	assert(msg.new_member.isValid());
	my_squad.push_back(msg.new_member);
}


/*
*                          *******                         
*/


DECL_OBJ_MANAGER("squad", TCompIASquadMember)

void TCompIASquadMember::load(const json& j, TEntityParseContext& ctx)
{
	if(!j.count("brain"))
		fatal("member has no brain");

	assert(j["brain"].is_string());
	brain = j.value("brain", "");
}

void TCompIASquadMember::onEntityCreated()
{
	// Create msg
	TMsgRegisterInSquad msg;
	CHandle owner = CHandle(this).getOwner();
	msg.new_member = owner;

	// Send msg
	CHandle e_brain = getEntityByName(brain);
	e_brain.sendMsg(msg);
}

void TCompIASquadMember::onPlayerSeen(const TMsgPlayerSeen& msg)
{
	if (!in_alarm)
	{
		in_alarm = true;
		dbg("player seen!");
	}
}

void TCompIASquadMember::update(float dt)
{

}

void TCompIASquadMember::debugInMenu()
{
	ImGui::Text("Brain: %s", brain.c_str());
}