#include "mcv_platform.h"
#include "module_boot.h"
#include "engine.h"
#include "module_entities.h"
#include "components/messages.h"
#include "render/gpu_culling_module.h"
#include "render/render_manager.h"
#include "render/render_module.h"
#include "components/common/comp_tags.h"
#include "modules/module_camera_mixer.h"
#include "modules/game/module_irradiance_cache.h"
#include "audio/module_audio.h"
#include "ui/ui_widget.h"
#include "ui/ui_module.h"
#include "components/common/comp_transform.h"
#include "components/abilities/comp_time_reversal.h"
#include "components/cameras/comp_camera_follow.h"

// #define USE_LOAD_THREAD

extern CShaderCte<CtesWorld> cte_world;
const int MAX_ENTRIES = 32;

static NamedValues<int>::TEntry character_entries[MAX_ENTRIES] = {
  {0, "preview_example"}
};

static NamedValues<int> output_names(character_entries, sizeof(character_entries) / sizeof(NamedValues<int>::TEntry));

bool CModuleBoot::start()
{
	if (jBoot.size())
		return true;

	jBoot = loadJson("data/boot.json");
	_loadPreview = jBoot.count("preview_mode") && jBoot["preview_mode"] == true;

	return true;
}

void CModuleBoot::update(float dt)
{
	if (_loadThread && _bootCompleted && !_bootReady)
	{
		if (_loadThread->joinable())
		{
			_loadThread->join();
			delete _loadThread;
			_loadThread = nullptr;
		}
		_bootReady = true;
	}

	if (_endBoot)
	{
		loadEndingBoot();
	}
}

void BootInThread(const std::string& boot_name, CModuleBoot* instance)
{
	assert(instance);
	instance->loadBoot(boot_name);
}

bool CModuleBoot::customStart()
{
	// Probably no loading screen..
	if (!jBoot.size())
	{
		jBoot = loadJson("data/boot.json");
		_loadPreview = jBoot.count("preview_mode") && jBoot["preview_mode"] == true;
		loadBoot("data/boot.json");
		return true;
	}

#ifdef USE_LOAD_THREAD
	_loadThread = new tbb::tbb_thread(&BootInThread, "data/boot.json", this);
#else
	loadBoot("data/boot.json");
#endif

	bool is_ok = Engine.getIrradiance().customStart();
	assert(is_ok);

	return is_ok;
}

bool CModuleBoot::loadEndingBoot()
{
	assert(jBoot.size());
	Boot.reset();
	auto prefabs = jBoot["happyRoom_scenes"].get<std::vector<std::string>>();
	for (auto& p : prefabs)
		loadScene(p);

	for (auto ctx : ctxs)
	{
		TMsgAllEntitiesCreated msg;
		for (auto h : ctx.entities_loaded)
			h.sendMsg(msg);
	}

	CEntity* camera_mixed = getEntityByName("camera_mixed");
	CModuleCameraMixer& mixer = CEngine::get().getCameramixer();
	mixer.setEnabled(true);
	mixer.setOutputCamera(camera_mixed);
	mixer.setDefaultCamera(getEntityByName("camera_follow"));
	mixer.blendCamera("camera_follow", 0.f);

	CEntity* e_camera_follow = getEntityByName("camera_follow");
	TCompCameraFollow* c_camera_follow = e_camera_follow->get<TCompCameraFollow>();
	c_camera_follow->enable();

	EngineRender.setActiveCamera(camera_mixed);
	EngineAudio.setListener(camera_mixed);

	// Spawn Eon in the player start position
	CEntity* player = getEntityByName("player");

	VHandles v_player_start = CTagsManager::get().getAllEntitiesByTag(getID("player_start"));

	assert(player);
	assert(!v_player_start.empty());
	
	// Be sure we have a player_start tag!
	CEntity* e_player_start = v_player_start[0];
	TCompTransform* h_start_trans = e_player_start->get<TCompTransform>();
	TCompTransform* h_player_trans = player->get<TCompTransform>();
	player->setPosition(h_start_trans->getPosition(), true);
	h_player_trans->setRotation(h_start_trans->getRotation());
			
	TCompTimeReversal* tr = player->get<TCompTimeReversal>();
	tr->disable();

	ui::CWidget* w_hud = EngineUI.getWidgetFrom("eon_hud", "warp_energy_bar");
	assert(w_hud);
	w_hud->setVisible(false);

	w_hud = EngineUI.getWidgetFrom("eon_hud", "warp_energy_bar_1");
	assert(w_hud);
	w_hud->setVisible(false);

	// restart irradiance module
	Engine.getIrradiance().restart();

	_endBoot = false;
	_bootCompleted = true;
	_bootReady = true;

	return true;
}

bool CModuleBoot::loadScene(const std::string& p, bool preloading)
{
	PROFILE_FUNCTION("loadScene");
	TEntityParseContext ctx;

	if (preloading) {
		CTransform t;
		t.setPosition(VEC3(0, -1e6, 0));
		ctx.root_transform = t;
	}

	dbg("Parsing %s\n", p.c_str());
	bool is_ok = parseScene(p, ctx);
	ctxs.push_back(ctx);

	return is_ok;
}

void CModuleBoot::loadBoot(const std::string& p)
{
	jBoot = loadJson(p);

	if (_introLoaded || !_introBoot)
	{
		if (_loadPreview)
		{
			loadPreviewBoot();
			return;
		}

		// Pre load resources
		if (jBoot.count("resources_to_load")) {

			_preloadingResources = true;

			auto prefabs = jBoot["resources_to_load"].get<std::vector<std::string>>();
			for (auto& p : prefabs)
				loadScene(p, true);

			for (auto& ctx : ctxs) {
				for (auto& h : ctx.entities_loaded)
					h.destroy();
			}

			ctxs.clear();
			CHandleManager::destroyAllPendingObjects();
			_preloadingResources = false;
		}

		auto prefabs = jBoot["scenes_to_load"].get<std::vector<std::string>>();
		for (auto& p : prefabs)
			loadScene(p);

		// We have finish parsing all the components of the entity
		for (auto ctx : ctxs)
		{
			TMsgAllEntitiesCreated msg;
			for (auto h : ctx.entities_loaded)
				h.sendMsg(msg);
		}
	}
	else
	{
		auto prefabs = jBoot["intro_scenes"].get<std::vector<std::string>>();
		for (auto& p : prefabs)
			loadScene(p);

		_introLoaded = true;
	}

	_bootCompleted = true;

	if (!_loadThread)
		_bootReady = true;
	
}

bool CModuleBoot::destroyBoot()
{
	auto prefabs = jBoot["scenes_to_load"].get<std::vector<std::string>>();
	for (auto it = rbegin(prefabs); it != rend(prefabs); ++it)
		destroyScene(*it);

	return true;
}

bool CModuleBoot::loadPreviewBoot()
{
	_loadPreview = false;
	_previewEnabled = true;

	loadBoot("data/scenes/3dviewer/boot.json");
	cte_world.boot_in_preview = 1.f;

	// load entries
	if (jBoot.count("entries")) {

		for (int i = 0; i < jBoot["entries"].size(); ++i) {
			const json& entry = jBoot["entries"][i];
			assert(entry.is_string());
			std::string entry_name = entry;

			int index = i + 1;
			character_entries[index] = { index, entry_name.c_str() };
			total_entries++;
		}

		output_names.set(character_entries, total_entries);
	}

	// We have finish parsing all the components of the entity
	for (auto ctx : ctxs)
	{
		TMsgAllEntitiesCreated msg;
		for (auto h : ctx.entities_loaded)
			h.sendMsg(msg);
	}

	// mouse state
	debugging = true;
	CApplication::get().changeMouseState(debugging, false);
	CApplication::get().setWndMouseVisible(true);

	// don't render debug
	Entities.clearDebugList();
	return true;
}

void CModuleBoot::reset()
{
	auto hm = getObjectManager<CEntity>();
	hm->forEach([](CEntity* e) {
		CHandle h(e);
		h.destroy();
		});

	CHandleManager::destroyAllPendingObjects();

	RenderManager.clearKeys();

	EngineCulling.reset();
	EngineCullingShadows.reset();

	_bootCompleted = false;
	_bootReady = false;
	ctxs.clear();
}

void CModuleBoot::renderInMenu()
{
	if (!_previewEnabled)
	{
		
	}

	else if (ImGui::TreeNode("Preview options"))
	{
		const char* options[MAX_ENTRIES] = {};
		for (int i = 0; i < total_entries; ++i) {
			options[i] = output_names.nameOf(i);
		}
		bool changed = ImGui::Combo("Entry", &_loaded, options, total_entries);

		if (changed)
		{
			std::string previous = output_names.nameOf(_lastLoaded);
			std::string to_remove = "data/scenes/3dviewer/" + previous + std::string(".json");
			destroyScene(to_remove);

			std::string next = output_names.nameOf(_loaded);
			std::string to_parse = "data/scenes/3dviewer/" + next + std::string(".json");
			bool is_ok = loadScene(to_parse);
			assert(is_ok);

			// We have finish parsing all the components of the entity
			for (auto ctx : ctxs)
			{
				TMsgAllEntitiesCreated msg;
				for (auto h : ctx.entities_loaded)
					h.sendMsg(msg);
			}

			_lastLoaded = _loaded;
		}

		ImGui::TreePop();
	}
}

void CModuleBoot::onFileChanged(const std::string& strfilename)
{
	int idx = 0;
	for (auto& ctx : ctxs) {

		if (ctx.filename == strfilename) {
			// Destroy previous entities
			for (auto h : ctx.all_entities_loaded)
				h.destroy();

			ctxs.erase(ctxs.begin() + idx);

			// and reload the file
			loadScene(strfilename);

			CHandleManager::destroyAllPendingObjects();
			return;
		}

		++idx;
	}
}
