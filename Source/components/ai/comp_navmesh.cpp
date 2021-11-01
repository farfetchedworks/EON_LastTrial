#include "mcv_platform.h"
#include "engine.h"
#include "render/draw_primitives.h"
#include "navmesh/module_navmesh.h"
#include "comp_navmesh.h"

DECL_OBJ_MANAGER("navmesh", TCompNavMesh)

void TCompNavMesh::load(const json& j, TEntityParseContext& ctx)
{
	if (!j.count("name")) {
		fatal("can't load navmesh: no name");
		return;
	}

	visible = j.value("visible", visible);

	const std::string& filename = j["name"];
	EngineNavMesh.load(filename);
}

void TCompNavMesh::onEntityCreated()
{
	_navMesh = EngineNavMesh.get();
}

void TCompNavMesh::onAllEntitiesCreated(const TMsgAllEntitiesCreated& msg) {}

void TCompNavMesh::debugInMenu()
{
	ImGui::Checkbox("Visible", &visible);
	if (ImGui::TreeNode("Query Tools")) {
		CNavMeshQuery::EQueryTool tool = _navMesh->getQueryTool();
		std::string name;
		switch (tool) {
		case CNavMeshQuery::EQueryTool::FIND_PATH:
			name = "Find path";
			break;
		case CNavMeshQuery::EQueryTool::WALL_DISTANCE:
			name = "Wall distance";
			break;
		case CNavMeshQuery::EQueryTool::RAYCAST:
			name = "Raycast";
			break;
		}
		ImGui::Text("Tool: %s", name.c_str());
		int iTool = (int)tool;
		if (ImGui::Combo("Select tool", &iTool, "Find path\0Wall distance\0Raycast\0"))
			_navMesh->setQueryTool((CNavMeshQuery::EQueryTool)iTool);
		ImGui::TreePop();
	}
}