#pragma once

#include "modules/module.h"
#include "navmesh/navmesh.h"

class CModuleNavMesh : public IModule
{
	CNavMesh* _currentNavMesh = nullptr;
	std::vector<CNavMesh*> _navMeshes;
	bool _renderDebug	= false;

public:
	CModuleNavMesh(const std::string& name) : IModule(name) {}
	~CModuleNavMesh();

	bool start() override;
	void stop() override;
	void render() override;
	void update(float dt) override;
	void renderDebug() override;
	void renderInMenu() override;

	CNavMesh* get(int idx = 0);
	bool setCurrent(const std::string& name);

	void getPath(VEC3 start, VEC3 end, TNavPath& path);
	float getWallDistance(VEC3 pos);
	bool raycast(VEC3 start, VEC3 end);
	VEC3 getRandomPoint();
	VEC3 getRandomPointAroundCircle(VEC3 pos, float radius);

	bool load(const std::string& filename);
	bool renderEnabled() { return _renderDebug; };
	void toggle();
};