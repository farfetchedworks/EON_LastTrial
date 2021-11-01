#pragma once
#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"
#include "DetourCommon.h"

class CNavMesh;

class CNavMeshQuery {
public:
    enum class EQueryTool {
        FIND_PATH = 0
        , WALL_DISTANCE
        , RAYCAST
        , NUM_TOOLS
    };

    struct TPos {
        VEC3 p = VEC3(0.f);
        bool set = false;
    };

    dtNavMesh* _data;
    dtNavMeshQuery* _q = nullptr;
    TPos    p1;
    TPos    p2;
    EQueryTool   tool;

public:
    ~CNavMeshQuery();
    void init(dtNavMesh* navMesh);
    void render();
    void update(float dt);

    // tools
    void setTool(EQueryTool atool);
    void resetTools();
    void updateTool();
    void findPath(VEC3 start, VEC3 end, std::vector<VEC3>& path);
    bool raycast(VEC3 start, VEC3 end);
    float wallDistance( VEC3 pos );
    VEC3 getRandomPoint();
    VEC3 getRandomPointAroundCircle(VEC3 pos, float radius);

private:

  CMesh* m_debugMesh = nullptr;

  // the dt data
  dtQueryFilter m_filter;
  dtStatus m_pathFindStatus;

  static const int MAX_POLYS = 256;
  static const int MAX_SMOOTH = 2048;

  dtPolyRef m_startRef;
  dtPolyRef m_endRef;
  dtPolyRef m_polys[ MAX_POLYS ];
  dtPolyRef m_parent[ MAX_POLYS ];
  int m_npolys;
  float m_straightPath[ MAX_POLYS * 3 ];
  unsigned char m_straightPathFlags[ MAX_POLYS ];
  dtPolyRef m_straightPathPolys[ MAX_POLYS ];
  int m_nstraightPath;
  float m_polyPickExt[ 3 ];
  float m_smoothPath[ MAX_SMOOTH * 3 ];
  int m_nsmoothPath;
  float m_queryPoly[ 4 * 3 ];

  static const int MAX_RAND_POINTS = 64;
  float m_randPoints[ MAX_RAND_POINTS * 3 ];
  int m_nrandPoints;
  bool m_randPointsInCircle;

  float m_hitPos[ 3 ];
  float m_hitNormal[ 3 ];
  bool m_hitResult;
  float m_distanceToWall;
  float m_neighbourhoodRadius;
  float m_randomRadius;

  int m_pathIterNum;
  dtPolyRef m_pathIterPolys[ MAX_POLYS ];
  int m_pathIterPolyCount;
  float m_prevIterPos[ 3 ], m_iterPos[ 3 ], m_steerPos[ 3 ], m_targetPos[ 3 ];

  static const int MAX_STEER_POINTS = 10;
  float m_steerPoints[ MAX_STEER_POINTS * 3 ];
  int m_steerPointCount;
};
