#include "mcv_platform.h"
#include "module_entities.h"
#include "entity/entity.h"
#include "components/common/comp_transform.h"
#include "components/common/comp_tags.h"
#include "render/draw_primitives.h"

void CModuleEntities::loadListOfManagers(const json& j, std::vector< CHandleManager* >& managers) {
	managers.clear();
	// For each entry in j["update"] add entry to om_to_update
	std::vector< std::string > names = j;
	for (auto& n : names) {
		auto om = CHandleManager::getByName(n.c_str());
		assert(om || fatal("Can't find a manager of components of type %s to update. Check file components.json\n", n.c_str()));
		managers.push_back(om);
	}
}


bool CModuleEntities::start()
{
	json j = loadJson("data/components.json");

	// Initialize the ObjManager preregistered in their constructors
	// with the amount of components defined in the data/components.json
	std::map< std::string, int > comp_sizes = j["sizes"];;
	int default_size = comp_sizes["default"];

	// Reorder the init manager based on the json
	// The bigger the number in the init_order section, the lower comp_type id you get
	std::map< std::string, int > init_order = j["init_order"];;
	std::sort(CHandleManager::predefined_managers
		, CHandleManager::predefined_managers + CHandleManager::npredefined_managers
		, [&init_order](CHandleManager* m1, CHandleManager* m2) {
			int priority_m1 = init_order[m1->getName()];
			int priority_m2 = init_order[m2->getName()];
			return priority_m1 > priority_m2;
		});
	// Important that the entity is the first one for the chain destruction of components
	assert(strcmp(CHandleManager::predefined_managers[0]->getName(), "entity") == 0);

	// Now with the sorted array
	for (size_t i = 0; i < CHandleManager::npredefined_managers; ++i) {
		auto om = CHandleManager::predefined_managers[i];
		auto it = comp_sizes.find(om->getName());
		int sz = (it == comp_sizes.end()) ? default_size : it->second;
		dbg("Initializing obj manager %s with %d\n", om->getName(), sz);
		om->init(sz, false);
	}

	loadListOfManagers(j["update"], om_to_update);
	loadListOfManagers(j["render_debug"], om_to_render_debug);

	if (j.count("multithread")) {
		std::vector< std::string > names = j["multithread"];
		for (auto& n : names) {
			auto om = CHandleManager::getByName(n.c_str());
			assert(om || fatal("Can't find a manager of components of type %s to update. Check file components.json\n", n.c_str()));
			om->multithreaded = true;
		}
	}

	return true;
}

void CModuleEntities::stop() {

	// Destroy all entities, should destroy all components in chain
	auto hm = getObjectManager<CEntity>();
	hm->forEach([](CEntity* e) {
		CHandle h(e);
		h.destroy();
		});

	CHandleManager::destroyAllPendingObjects();
}

void CModuleEntities::update(float delta) {

	for (auto om : om_to_update) {
		PROFILE_FUNCTION(om->getName());
		om->updateAll(delta);

		// In case one component destroy an entity, this will remove all the sibling components 
		// before we update the rest of the components
		CHandleManager::destroyAllPendingObjects();
	}

}

void CModuleEntities::renderInMenu() {

	if (ImGui::TreeNode("All Entities...")) {

		ImGui::InputText("Search", buff, 128);
		ImGui::Separator();

		auto om = getObjectManager<CEntity>();
		om->forEach([&](CEntity* e) {
			CHandle h(e);
			if (h.getOwner().isValid())
				return;

			std::string buffString = buff;
			std::string name = e->getName();

			toLowerCase(buffString);
			toLowerCase(name);

			if (name.size() > 0 && name.find(buffString) == std::string::npos)
				return;

			ImGui::PushID(e);
			e->debugInMenu();
			ImGui::PopID();
		});

		ImGui::TreePop();
	}

	CTagsManager::get().renderInMenu();


	editRenderDebug();
}

void CModuleEntities::renderDebug() {

	// Ensure we have this pipeline color + pos to render the debug geometries
	auto pipeline_debug = Resources.get("debug.pipeline")->as<CPipelineState>();

	for (auto om : om_to_render_debug) {
		CGpuScope gpu_scope(om->getName());
		pipeline_debug->activate();
		om->renderDebugAll();
	}
}

void CModuleEntities::render() {
}

void CModuleEntities::renderDebugOfComponents() {

}

void CModuleEntities::editRenderDebug() {
	if (ImGui::TreeNode("Render Debug...")) {

		ImGui::SameLine();
		if (ImGui::SmallButton("None"))
			clearDebugList();

		std::vector< CHandleManager* > managers;
		for (uint32_t i = 1; i < CHandleManager::getNumDefinedTypes(); ++i)
			managers.push_back(CHandleManager::getByType(i));

		std::sort(managers.begin(), managers.end(), [](const CHandleManager* om1, const CHandleManager* om2) {
			return (strcmp(om1->getName(), om2->getName()) < 0);
			});

		// Render the amangers in 3 columns
		ImGui::Columns(3, NULL, true);
		for (auto om : managers) {

			// Check if this om is in the list of manager to 'debug render'
			auto it = std::find(om_to_render_debug.begin(), om_to_render_debug.end(), om);
			bool is_visible = it != om_to_render_debug.end();

			if (ImGui::Checkbox(om->getName(), &is_visible)) {
				if (is_visible) {   // Now it's not visible
					om_to_render_debug.push_back(om);
				}
				else {
					om_to_render_debug.erase(it);
				}
			}
			ImGui::NextColumn();
		}

		ImGui::Columns(1);
		ImGui::TreePop();
	}
}

void CModuleEntities::clearDebugList()
{
	om_to_render_debug.clear();
}

void CModuleEntities::enableBasicDebug()
{
	const json jData = loadJson("data/ui/render_debug.json");
	for (const json& jFileEntry : jData)
	{
		const std::string render_mgr_name_a = jFileEntry;
		om_to_render_debug.push_back(CHandleManager::getByName(render_mgr_name_a.c_str()));
	}
}