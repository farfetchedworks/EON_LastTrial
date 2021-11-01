#include "mcv_platform.h"
#include "module_boot.h"
#include "engine.h"
#include "module_entities.h"
#include "components/messages.h"

extern CShaderCte<CtesWorld> cte_world;
const int MAX_ENTRIES = 32;

static NamedValues<int>::TEntry character_entries[MAX_ENTRIES] = {
  {0, "preview_example"}
};

static NamedValues<int> output_names(character_entries, sizeof(character_entries) / sizeof(NamedValues<int>::TEntry));

bool CModuleBoot::start()
{
	if (jBoot.size()) {
		return true;
	}

	return loadBoot("data/subvert_boot.json");
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

bool CModuleBoot::loadBoot(const std::string& p)
{
	jBoot = loadJson(p);

	// If preview enabled, forget about loading the game
	if (jBoot.count("preview_mode") && jBoot["preview_mode"])
	{
		return loadPreviewBoot();
	}
	
	// Pre load resources
	if(jBoot.count("resources_to_load")){

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

		// TODO: Remove GPU Instances
		{

		}

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

	return true;
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
	bool is_ok = loadBoot("data/scenes/3dviewer/boot.json");
	previewEnabled = true;
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

	// don't render debug
	Entities.clearDebugList();
	return is_ok;
}

void CModuleBoot::renderInMenu()
{
	if (!previewEnabled)
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
