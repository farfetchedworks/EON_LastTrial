#pragma once
#include "handle/handle.h"
#include "entity/entity.h"
#include "components/common/comp_base.h"
#include "components/messages.h"

/*
- CompIASquadBrain{
	// Array of CHandles 
	std::vector<CHandle> my_squad;
	bool in_alarm = false

	- on new member

	- on PlayerSeen {
		-If not in alarm->send MsgPlayerSeen to all members of my squad
	}

- CompIASquadMember {
	-On Start send a msg to your brain to register youself
	- If not in alarm
		Use the transform to check if see the player in our cone vision
		If YES->Send msg MsgPlayerSeen to the SquadBrain entity about the location of the player.
	- If the Brain send us a MsgPlayerSeen, enter in alarm state
	  and orient to the target position
}*/

class TCompIASquadBrain : public TCompBase {

	DECL_SIBLING_ACCESS();

	std::vector<CHandle> my_squad;
	bool in_alarm = false;

public:

	static void registerMsgs() {
		DECL_MSG(TCompIASquadBrain, TMsgPlayerSeen, onPlayerSeen);
		DECL_MSG(TCompIASquadBrain, TMsgRegisterInSquad, onNewMember);
	}

	void onPlayerSeen(const TMsgPlayerSeen& msg);
	void onNewMember(const TMsgRegisterInSquad& msg);
};

class TCompIASquadMember : public TCompBase {

	DECL_SIBLING_ACCESS();

	std::string brain;
	bool in_alarm = false;

public:

	static void registerMsgs() {
		DECL_MSG(TCompIASquadMember, TMsgPlayerSeen, onPlayerSeen);
	}

	void load(const json& j, TEntityParseContext& ctx);
	void onEntityCreated();
	void onPlayerSeen(const TMsgPlayerSeen& msg);

	void debugInMenu();
	void update(float dt);
};