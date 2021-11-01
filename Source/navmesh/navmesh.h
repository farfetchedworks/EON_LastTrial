#pragma once
#include "resources/resources.h"
#include "Recast.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"
#include "navmesh_query.h"

struct NavMeshVertex;

const int MAX_SEARCH_NODES = 2048;
//const int MAX_POLYS = 256;
//const int MAX_SMOOTH = 2048;
//const int MAX_AGENTS = 25;
//const float MAX_AGENT_RADIUS = 2.f;

using TNavPath = std::vector<VEC3>;

class CNavMesh : public IResource {

    friend class CNavMeshQuery;

    bool m_debugEnabled = true;

    dtNavMesh* m_navMesh;
    CNavMeshQuery m_navQuery;

    CMesh* m_debugMesh = nullptr;
    CMesh* m_debugBoundsMesh = nullptr;

    dtNavMesh* parse(CFileDataProvider& dp);
    void fillDebugMesh(std::vector<NavMeshVertex>& vertices);
    void fillDebugBoundsMesh(std::vector<NavMeshVertex>& vertices);

    static void vertex(std::vector<NavMeshVertex>& vertices, const float* position, VEC4 color);
    static VEC4 areaToColor(unsigned int area);
    static void drawPolyBoundaries(std::vector<NavMeshVertex>& vertices, const dtMeshTile* tile, 
        VEC4 color, bool inner);

public:

    // poner las que queramos nosotros
    enum SamplePolyAreas
    {
        SAMPLE_POLYAREA_GROUND,
        SAMPLE_POLYAREA_WATER,
        SAMPLE_POLYAREA_ROAD,
        SAMPLE_POLYAREA_DOOR,
        SAMPLE_POLYAREA_GRASS,
        SAMPLE_POLYAREA_JUMP,
    };

    enum {
        FLAG_WALK = 0x01
        , FLAG_SWIM = 0x02
        , FLAG_DISABLED = 0x10
        , ALL_FLAGS = 0xffff
    };

    CNavMeshQuery::EQueryTool getQueryTool() { return m_navQuery.tool; }
    void setQueryTool(CNavMeshQuery::EQueryTool t) { m_navQuery.tool = t; }
    CNavMeshQuery* getQuery() { return &m_navQuery; };

    ~CNavMesh();
    const std::string& getName() { return name; }
    bool load(CFileDataProvider& dp);
    void update(float dt);
    void renderInMenu();
    void renderDebug();
};