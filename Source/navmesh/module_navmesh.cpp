#include "mcv_platform.h"
#include "engine.h"
#include "module_navmesh.h"
#include "modules/module_boot.h"

CModuleNavMesh::~CModuleNavMesh()
{
	for (auto n : _navMeshes) {
		n->destroy();
		n = nullptr;
	}

	_currentNavMesh = nullptr;
}

bool CModuleNavMesh::start()
{
	return true;
}

void CModuleNavMesh::stop() {}

bool CModuleNavMesh::setCurrent(const std::string& name)
{
	for (auto n : _navMeshes) {
		std::string full_path = "data/nav_meshes/" + name + ".navmesh";
		if (full_path == n->getName()) {
			_currentNavMesh = n;
			return true;
		}
	}

	return false;
}

CNavMesh* CModuleNavMesh::get(int idx)
{
	if (idx < 0)
		return _currentNavMesh;
	else if(_navMeshes.size() > idx)
		return _navMeshes[idx];

	return nullptr;
}

void CModuleNavMesh::update(float dt)
{
	if (_currentNavMesh)
		_currentNavMesh->update(dt);
}

bool CModuleNavMesh::load(const std::string& filename)
{
	CNavMesh* _navMesh = const_cast<CNavMesh*>(Resources.get(filename)->as<CNavMesh>());
	assert(_navMesh);
	if (!_navMesh) {
		return false;
	}

	// Set first if not preload steps and no current
	if (!Boot.isPreloading() && !_currentNavMesh) {
		_currentNavMesh = _navMesh;
	}

	_navMeshes.push_back(_navMesh);

	return true;
}

void CModuleNavMesh::toggle()
{
	_renderDebug = !_renderDebug;
}

void CModuleNavMesh::getPath(VEC3 start, VEC3 end, TNavPath& path)
{
	if(_currentNavMesh)
		_currentNavMesh->getQuery()->findPath(start, end, path);
}

float CModuleNavMesh::getWallDistance(VEC3 pos)
{
	if (_currentNavMesh)
		return _currentNavMesh->getQuery()->wallDistance(pos);
	return -1.f;
}

bool CModuleNavMesh::raycast(VEC3 start, VEC3 end)
{
	if(_currentNavMesh)
		return _currentNavMesh->getQuery()->raycast(start, end);
	return false;
}

VEC3 CModuleNavMesh::getRandomPoint()
{
	if (_currentNavMesh)
		return _currentNavMesh->getQuery()->getRandomPoint();
	return VEC3(-1.f);
}

VEC3 CModuleNavMesh::getRandomPointAroundCircle(VEC3 pos, float radius)
{
	if (_currentNavMesh)
		return _currentNavMesh->getQuery()->getRandomPointAroundCircle(pos, radius);
	return VEC3(-1.f);
}

void CModuleNavMesh::render() {}

void CModuleNavMesh::renderInMenu()
{
    if (ImGui::TreeNode("Navmesh"))
    {
		if (_currentNavMesh) {
			ImGui::Text("Current: %s", _currentNavMesh->getName().c_str());
		}

        if (ImGui::TreeNode("All"))
        {
			for (auto n : _navMeshes)
			{
				ImGui::PushID(n);

				if (ImGui::Button("Set current")) {
					_currentNavMesh = n;
				}

				n->renderInMenu();
				ImGui::Separator();
				ImGui::PopID();
			}

            ImGui::TreePop();
        }

        ImGui::TreePop();
    }
}

void CModuleNavMesh::renderDebug()
{
	if (_renderDebug && _currentNavMesh) {
		_currentNavMesh->renderDebug();
	}
}
