#include "mcv_platform.h"
#include "engine.h"
#include "module_scripting.h"
#include "modules/module_events.h"
#include "audio/module_audio.h"
#include "logic_manager.h"
#include "lua_console.h"
#include "entity/entity.h"
#include "entity/entity_parser.h"
#include "components/cameras/comp_camera_shake.h"
#include "components/common/comp_collider.h"
#include "components/stats/comp_geons_manager.h"

CModuleScripting::~CModuleScripting()
{
	pending_scripts.clear();
}

bool CModuleScripting::start()
{
	console = new LuaConsole([&](char* s) {
		std::string command = std::string(s);
		bool is_ok = executeScript(command);
		return is_ok;
	});

	// Add console commands
	registerLuaConsoleCommands();

	// Bind everything needed
	bindLua();

	// Boot LUA globals vars
	bool is_ok = executeFile("data/scripts/boot.lua");
	is_ok &= executeFile("data/scripts/utils.lua");
	is_ok &= executeFile("data/scripts/coroutines.lua");
	is_ok &= executeFile("data/scripts/cinematics.lua");
	is_ok &= executeFile("data/scripts/trigger_areas.lua");
	is_ok &= executeFile("data/scripts/audio/event_registrations.lua");

	assert(is_ok);
	return is_ok;
}

void CModuleScripting::bindLua()
{
	// Libraries

	lua_state.open_libraries(sol::lib::base);
	lua_state.open_libraries(sol::lib::math);
	lua_state.open_libraries(sol::lib::coroutine);
	lua_state.open_libraries(sol::lib::package);
	lua_state.open_libraries(sol::lib::table);

	// Class Objects

	lua_state.new_usertype<VEC3>(
		"VEC3", sol::constructors<VEC3(), VEC3(float), VEC3(float, float, float)>(),
		"x", &VEC3::x, "y", &VEC3::y, "z", &VEC3::z,
		"Dot", &VEC3::Dot, 
		"Normalize", sol::resolve<void()>(&VEC3::Normalize),
		sol::meta_function::addition, sol::resolve<VEC3(const VEC3&, const VEC3&)>(&DirectX::SimpleMath::operator+),
		sol::meta_function::subtraction, sol::resolve <VEC3(const VEC3&, const VEC3&)>(&DirectX::SimpleMath::operator-),
		sol::meta_function::multiplication, sol::overload(
			sol::resolve<VEC3(const VEC3&, const VEC3&)>(&DirectX::SimpleMath::operator*),
			sol::resolve<VEC3(const VEC3&, const float)>(&DirectX::SimpleMath::operator*)
		),
		sol::meta_function::division, sol::overload(
			sol::resolve<VEC3(const VEC3&, const VEC3&)>(&DirectX::SimpleMath::operator/),
			sol::resolve<VEC3(const VEC3&, const float)>(&DirectX::SimpleMath::operator/)
		),
		sol::meta_function::equal_to, &VEC3::operator==
	);

	lua_state.new_usertype<CHandle>(
		"Handle", sol::constructors<CHandle()>(),
		"destroy", &CHandle::destroy,
		"isValid", &CHandle::isValid,
		"getOwner", &CHandle::getOwner
	);

	// Utility functions

	lua_state.set("dbg", &LogicManager::ldbg);
	lua_state.set("activateWidget", &LogicManager::activateWidget);
	lua_state.set("deactivateWidget", &LogicManager::deactivateWidget);
	lua_state.set("fade", &LogicManager::fade);
	lua_state.set("goToGamestate", &LogicManager::goToGamestate);
	lua_state.set("startCinematic", &LogicManager::startCinematic);
	lua_state.set("startCinematicAnimation", &LogicManager::startCinematicAnimation);
	lua_state.set("startStaticCinematicAnimation", &LogicManager::startStaticCinematicAnimation);
	lua_state.set("stopCinematic", &LogicManager::stopCinematic);
	lua_state.set("setCinematicCurve", &LogicManager::setCinematicCurve);
	lua_state.set("setCinematicTarget", &LogicManager::setCinematicTarget);
	lua_state.set("setCinematicTargetEntity", &LogicManager::setCinematicTargetEntity);
	lua_state.set("getEntityPosByName", &LogicManager::getEntityPosByName);

	// Entities

	lua_state.set("getEntityByName", &getEntityByName);

	// Events

	lua_state.set("registerEvent", sol::overload(
		[](const std::string& event_name, sol::function fn) {
			return EventSystem.registerEventCallback(event_name, CHandle(), fn);
		},
		[](const std::string& event_name, CHandle obs, sol::function fn) {
			return EventSystem.registerEventCallback(event_name, obs, fn);
		})
	);

	lua_state.set("removeEvent", [](const std::string& event_name, unsigned int id) {
		EventSystem.unregisterEventCallback(event_name, id);
	});
	
	lua_state.set("dispatchEvent", sol::overload(
		[](const std::string& event_name) {
			EventSystem.dispatchEvent(event_name);
		},
		[](const std::string& event_name, CHandle trigger) {
			EventSystem.dispatchEvent(event_name, trigger);
		})
	);

	// Audio

	lua_state.set("fmodPostEvent", sol::overload(
		[](const std::string& ev_name) {
			return EngineAudio.postEvent(ev_name);
		},
		[](const std::string& ev_name, VEC3 location) {
			return EngineAudio.postEvent(ev_name, location);
		},
		[](const std::string& ev_name, CHandle actor) {
			return EngineAudio.postEvent(ev_name, actor);
		})
	);

	lua_state.set("fmodPostFloorEvent", [](const std::string& ev_name, CHandle actor) {
		EngineAudio.postFloorEvent(ev_name, actor);
	});

	lua_state.set("fmodPostMusicEvent", [](const std::string& ev_name) {
		EngineAudio.postMusicEvent(ev_name);
	});

	lua_state.set("fmodLoadBank", [](const std::string& bank_name) {
		EngineAudio.loadBank(bank_name);
	});

	lua_state.set("fmodUnloadBank", [](const std::string& bank_name) {
		EngineAudio.unloadBank(bank_name);
	});

	lua_state.set("fmodSetGlobalRTPC", [](const std::string& param_name, const float value) {
		EngineAudio.setGlobalRTPC(param_name, value);
	});

	lua_state.set("fmodSetListener", [](CHandle actor) {
		EngineAudio.setListener(actor);
	});

	// Cameras

	lua_state.set("shakeOnce", sol::overload(
		[](float amount, float fadeIn, float fadeOut) {
			CEntity* outputCamera = getEntityByName("camera_mixed");
			TCompCameraShake* shaker = outputCamera->get<TCompCameraShake>();
			if(shaker)
				shaker->shakeOnce(amount, fadeIn, fadeOut);
		},
		[](float amount, float fadeIn, float fadeOut, bool is_3d) {
			CEntity* outputCamera = getEntityByName("camera_mixed");
			TCompCameraShake* shaker = outputCamera->get<TCompCameraShake>();
			if (shaker)
				shaker->shakeOnce(amount, fadeIn, fadeOut, is_3d);
		}
	));

	// Colliders

	lua_state.set("setColliderStatus", [](CHandle owner, bool status) {
		assert(owner.isValid());
		CEntity* e_owner = owner;
		TCompCollider* collider = e_owner->get<TCompCollider>();
		if (collider) {
			collider->disable(!status);
		}
	});

	// Stats

	lua_state.set("addGeons", [](int n) {
		CEntity* player = getEntityByName("player");
		assert(player);
		TCompGeonsManager* gManager = player->get<TCompGeonsManager>();
		if (gManager) {
			gManager->addGeons(n);
		}
	});
}

void CModuleScripting::stop()
{
	delete console;
}

void CModuleScripting::update(float dt)
{
	bool removePending = false;

	for (int i = 0; i < pending_scripts.size(); ++i)
	{
		TScript& s = pending_scripts[i];

		s.delay -= dt;

		if (s.delay <= 0.f)
		{
			bool is_ok = executeScript(s.lua_code);
			assert(is_ok && "Can't execute delayed lua script");
		}
	}

	char buffer[64];
	snprintf(buffer, 64, "updateCoroutines(%f)", dt);
	executeScript(buffer);

	// Remove executed scripts
	auto it = std::remove_if(begin(pending_scripts), end(pending_scripts), [&](TScript sc) {
		return sc.delay <= 0.f;
	});

	pending_scripts.erase(it, end(pending_scripts));
}

bool CModuleScripting::executeScript(const std::string& script, float delay)
{
	if (delay > 0.f)
	{
		static unsigned int uids = 0;
		TScript dScript = { uids++, script, delay };
		pending_scripts.push_back(dScript);
	}
	else
	{
		auto result = lua_state.safe_script(script, &sol::script_pass_on_error);
		return result.valid(); 
	}

	return true;
}

bool CModuleScripting::executeFile(const std::string& filename)
{
	auto result = lua_state.safe_script_file(filename);
	return result.valid();
}

void CModuleScripting::renderInMenu()
{
	if(lc_opened)
		console->Draw("Lua Console", lc_opened);
}

void CModuleScripting::logConsole(const char* log)
{
	console->AddLog(log);
}

void CModuleScripting::registerLuaConsoleCommands()
{
	const char* cmds[] = {
		"new()",
		"dbg()",
		"VEC3",
		"Handle",
		"registerEvent()",
		"dispatchEvent()",
		"removeEvent()",
		"getEntityByName()",
		"destroyEntity()"
	};

	for (auto& cmd : cmds)
		console->Commands.push_back(cmd);
}