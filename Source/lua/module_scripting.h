#pragma once
#define SOL_ALL_SAFETIES_ON 1
#include "modules/module.h"
#include "lua/sol/sol.hpp"

struct LuaConsole;

class CModuleScripting : public IModule {

	struct TScript
	{
		unsigned int uid;
		std::string lua_code;
		float delay;
	};

public:
	
	sol::state lua_state;

	CModuleScripting(const std::string& name) : IModule(name) { }
	~CModuleScripting();

	bool start() override;
	void stop() override;
	void update(float delta) override;
	void renderInMenu() override;

	void bindLua();
	void registerLuaConsoleCommands();

	bool executeFile(const std::string& filename);
	bool executeScript(const std::string& script, float delay = 0.f);

	void logConsole(const char* log);
	void openConsole() { lc_opened = true; };

private:
	std::vector<TScript> pending_scripts;

	LuaConsole* console = nullptr;
	bool lc_opened		= false;
};