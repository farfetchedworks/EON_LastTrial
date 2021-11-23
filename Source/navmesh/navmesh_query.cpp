#include "mcv_platform.h"
#include "engine.h"
#include "navmesh.h"
#include "entity/entity.h"
#include "navmesh_query.h"
#include "DetourDebugDraw.h"
#include "render/draw_primitives.h"
#include "input/input_module.h"

CNavMeshQuery::~CNavMeshQuery()
{
    dtFreeNavMeshQuery(_q);
}

void CNavMeshQuery::init(dtNavMesh* navMesh)
{
    _q = dtAllocNavMeshQuery();
    _q->init(navMesh, MAX_SEARCH_NODES);

    // store navmesh pointer
    _data = navMesh;

    if (m_debugMesh) {
        m_debugMesh->destroy();
        delete m_debugMesh;
    }

    resetTools();
}

void CNavMeshQuery::render()
{
    if (!p1.set) return;

    // Start, end points
    drawWiredSphere(MAT44::CreateTranslation(p1.p), 0.1f);

    if (tool == EQueryTool::FIND_PATH)
    {
        drawWiredSphere(MAT44::CreateTranslation(p2.p), 0.1f);
        if (m_debugMesh)
            drawPrimitive(m_debugMesh, MAT44::Identity, Colors::Black, RSConfig::WIREFRAME);
    }
    else if (tool == EQueryTool::RAYCAST)
    {
        drawWiredSphere(MAT44::CreateTranslation(p2.p), 0.1f);
        VEC3 hitPos = VEC3(m_hitPos[0], m_hitPos[1], m_hitPos[2]) + VEC3::Up;

        // can't go straight to the end point
        if (m_hitResult) 
        {
            drawLine(p1.p, hitPos, Colors::Red);
            drawLine(hitPos, p2.p, Colors::Green);
        }
        else {
            drawLine(p1.p, p2.p, Colors::Red);
        }
    }
}

void CNavMeshQuery::update(float dt)
{
    /*if (PlayerInput['0'].getsPressed()) {
        tool = (EQueryTool)(((int)tool + 1) % (int)EQueryTool::NUM_TOOLS);
    }
    else if( PlayerInput['1'].getsPressed() ) {

        CEntity* player = getEntityByName("player");

        p1.p = player->getPosition() + VEC3::Up;
        p1.set = true;
        updateTool();

    }
    else if (PlayerInput['2'].getsPressed()) {

        CEntity* player = getEntityByName("player");

        p2.p = player->getPosition() + VEC3::Up;
        p2.set = true;
        updateTool();
    }
    else if (PlayerInput['3'].getsPressed()) {

        p2.set = false;
        p1.p = getRandomPoint() + VEC3::Up;
        p1.set = true;
    }
    else if (PlayerInput['4'].getsPressed()) {

        CEntity* player = getEntityByName("player");
        p2.set = false;
        p1.p = getRandomPointAroundCircle(player->getPosition(), m_neighbourhoodRadius) + VEC3::Up;
        p1.set = true;
    }*/
}

// =============================================================== TOOLS
void CNavMeshQuery::setTool( EQueryTool atool ) {
  tool = atool;
}

void CNavMeshQuery::resetTools( ) {
  m_startRef = 0;
  m_endRef = 0;
  m_npolys = 0;
  m_nstraightPath = 0;
  m_nsmoothPath = 0;
  memset( m_hitPos, 0, sizeof( m_hitPos ) );
  memset( m_hitNormal, 0, sizeof( m_hitNormal ) );
  m_distanceToWall = 0;

  /*m_filter.setIncludeFlags( 0xffff );
  m_filter.setExcludeFlags( 0 );*/

  m_polyPickExt[ 0 ] = 2.f;
  m_polyPickExt[ 1 ] = 8.f;
  m_polyPickExt[ 2 ] = 2.f;

  m_neighbourhoodRadius = 2.5f;
  m_randomRadius = 5.0f;

  tool = EQueryTool::FIND_PATH;
}

void CNavMeshQuery::updateTool()
{
    // update the current tool
    if (tool == EQueryTool::FIND_PATH)
    {
        if (!p1.set || !p2.set) return;
        
        std::vector<VEC3> path = {};
        findPath(p1.p, p2.p, path);

        if (!path.size()) return;

        for (auto& p : path) {
            p.y += 0.9f;
        }

        if (!m_debugMesh)
            m_debugMesh = new CMesh();

        // Clean mesh
        m_debugMesh->destroy();
        bool is_ok = m_debugMesh->create(path.data(), (uint32_t)path.size() * sizeof(VEC3), sizeof(VEC3), nullptr,
            0, 0, D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP, getVertexDeclarationByName("Pos"));
        assert(is_ok);
    }
    else if (tool == EQueryTool::WALL_DISTANCE)
    {
        if (!p1.set) return;

        p2.set = false;
        float dist = wallDistance(p1.p);
        dbg("Distance to wall is: %f\n", dist);
    }
    else if (tool == EQueryTool::RAYCAST) {
        if(p1.set && p2.set) raycast(p1.p, p2.p);
    }
}

// --------------------------------------
// --------------------------------------
inline bool inRange( const float* v1, const float* v2, const float r, const float h ) {
  const float dx = v2[ 0 ] - v1[ 0 ];
  const float dy = v2[ 1 ] - v1[ 1 ];
  const float dz = v2[ 2 ] - v1[ 2 ];
  return (dx*dx + dz*dz) < r*r && fabsf( dy ) < h;
}

static int fixupCorridor( dtPolyRef* path, const int npath, const int maxPath,
                          const dtPolyRef* visited, const int nvisited ) {
  int furthestPath = -1;
  int furthestVisited = -1;

  // Find furthest common polygon.
  for( int i = npath - 1; i >= 0; --i ) {
    bool found = false;
    for( int j = nvisited - 1; j >= 0; --j ) {
      if( path[ i ] == visited[ j ] ) {
        furthestPath = i;
        furthestVisited = j;
        found = true;
      }
    }
    if( found )
      break;
  }

  // If no intersection found just return current path. 
  if( furthestPath == -1 || furthestVisited == -1 )
    return npath;

  // Concatenate paths.	

  // Adjust beginning of the buffer to include the visited.
  const int req = nvisited - furthestVisited;
  const int orig = rcMin( furthestPath + 1, npath );
  int size = rcMax( 0, npath - orig );
  if( req + size > maxPath )
    size = maxPath - req;
  if( size )
    memmove( path + req, path + orig, size*sizeof( dtPolyRef ) );

  // Store visited
  for( int i = 0; i < req; ++i )
    path[ i ] = visited[ (nvisited - 1) - i ];

  return req + size;
}

// This function checks if the path has a small U-turn, that is,
// a polygon further in the path is adjacent to the first polygon
// in the path. If that happens, a shortcut is taken.
// This can happen if the target (T) location is at tile boundary,
// and we're (S) approaching it parallel to the tile edge.
// The choice at the vertex can be arbitrary, 
//  +---+---+
//  |:::|:::|
//  +-S-+-T-+
//  |:::|   | <-- the step can end up in here, resulting U-turn path.
//  +---+---+
static int fixupShortcuts( dtPolyRef* path, int npath, dtNavMeshQuery* navQuery ) {
  if( npath < 3 )
    return npath;

  // Get connected polygons
  static const int maxNeis = 16;
  dtPolyRef neis[ maxNeis ];
  int nneis = 0;

  const dtMeshTile* tile = 0;
  const dtPoly* poly = 0;
  if( dtStatusFailed( navQuery->getAttachedNavMesh( )->getTileAndPolyByRef( path[ 0 ], &tile, &poly ) ) )
    return npath;

  for( unsigned int k = poly->firstLink; k != DT_NULL_LINK; k = tile->links[ k ].next ) {
    const dtLink* link = &tile->links[ k ];
    if( link->ref != 0 ) {
      if( nneis < maxNeis )
        neis[ nneis++ ] = link->ref;
    }
  }

  // If any of the neighbour polygons is within the next few polygons
  // in the path, short cut to that polygon directly.
  static const int maxLookAhead = 6;
  int cut = 0;
  for( int i = dtMin( maxLookAhead, npath ) - 1; i > 1 && cut == 0; i-- ) {
    for( int j = 0; j < nneis; j++ ) {
      if( path[ i ] == neis[ j ] ) {
        cut = i;
        break;
      }
    }
  }
  if( cut > 1 ) {
    int offset = cut - 1;
    npath -= offset;
    for( int i = 1; i < npath; i++ )
      path[ i ] = path[ i + offset ];
  }

  return npath;
}

static bool getSteerTarget( dtNavMeshQuery* navQuery, const float* startPos, const float* endPos,
                            const float minTargetDist,
                            const dtPolyRef* path, const int pathSize,
                            float* steerPos, unsigned char& steerPosFlag, dtPolyRef& steerPosRef,
                            float* outPoints = 0, int* outPointCount = 0 ) {
  // Find steer target.
  static const int MAX_STEER_POINTS = 3;
  float steerPath[ MAX_STEER_POINTS * 3 ];
  unsigned char steerPathFlags[ MAX_STEER_POINTS ];
  dtPolyRef steerPathPolys[ MAX_STEER_POINTS ];
  int nsteerPath = 0;
  navQuery->findStraightPath( startPos, endPos, path, pathSize,
                              steerPath, steerPathFlags, steerPathPolys, &nsteerPath, MAX_STEER_POINTS );
  if( !nsteerPath )
    return false;

  if( outPoints && outPointCount ) {
    *outPointCount = nsteerPath;
    for( int i = 0; i < nsteerPath; ++i )
      dtVcopy( &outPoints[ i * 3 ], &steerPath[ i * 3 ] );
  }


  // Find vertex far enough to steer to.
  int ns = 0;
  while( ns < nsteerPath ) {
    // Stop at Off-Mesh link or when point is further than slop away.
    if( (steerPathFlags[ ns ] & DT_STRAIGHTPATH_OFFMESH_CONNECTION) ||
        !inRange( &steerPath[ ns * 3 ], startPos, minTargetDist, 1000.0f ) )
        break;
    ns++;
  }
  // Failed to find good point to steer to.
  if( ns >= nsteerPath )
    return false;

  dtVcopy( steerPos, &steerPath[ ns * 3 ] );
  steerPos[ 1 ] = startPos[ 1 ];
  steerPosFlag = steerPathFlags[ ns ];
  steerPosRef = steerPathPolys[ ns ];

  return true;
}
// --------------------------------------
// --------------------------------------

void CNavMeshQuery::findPath(VEC3 start, VEC3 end, std::vector<VEC3>& path)
{
    m_pathFindStatus = DT_FAILURE;
    m_pathIterNum = 0;

    memset(m_smoothPath, 0, MAX_SMOOTH * 3);

    dtStatus status = DT_SUCCESS;
    status &= _q->findNearestPoly(&start.x, m_polyPickExt, &m_filter, &m_startRef, 0);
    assert(dtStatusSucceed(status));

    status &= _q->findNearestPoly(&end.x, m_polyPickExt, &m_filter, &m_endRef, 0);
    assert(dtStatusSucceed(status));

    _q->findPath(m_startRef, m_endRef, &start.x, &end.x, &m_filter, m_polys, &m_npolys, MAX_POLYS);

    m_nsmoothPath = 0;

    if (m_npolys)
    {
        // Iterate over the path to find smooth path on the detail mesh surface.
        dtPolyRef polys[MAX_POLYS];
        memcpy(polys, m_polys, sizeof(dtPolyRef) * m_npolys);
        int npolys = m_npolys;

        float iterPos[3], targetPos[3];

        status &= _q->closestPointOnPoly(m_startRef, &start.x, iterPos, 0);
        status &= _q->closestPointOnPoly(polys[npolys - 1], &end.x, targetPos, 0);
        assert(dtStatusSucceed(status));

        static const float STEP_SIZE = 1.f;
        static const float SLOP = 0.1f;

        m_nsmoothPath = 0;

        dtVcopy(&m_smoothPath[m_nsmoothPath * 3], iterPos);
        m_nsmoothPath++;
        path.push_back(VEC3(iterPos[0], iterPos[1], iterPos[2]));

        // Move towards target a small advancement at a time until target reached or
        // when ran out of memory to store the path.
        while (npolys && m_nsmoothPath < MAX_SMOOTH) {
            // Find location to steer towards.
            float steerPos[3];
            unsigned char steerPosFlag;
            dtPolyRef steerPosRef;

            if (!getSteerTarget(_q, iterPos, targetPos, SLOP,
                polys, npolys, steerPos, steerPosFlag, steerPosRef))
                break;

            bool endOfPath = (steerPosFlag & DT_STRAIGHTPATH_END) ? true : false;
            bool offMeshConnection = (steerPosFlag & DT_STRAIGHTPATH_OFFMESH_CONNECTION) ? true : false;

            // Find movement delta.
            float delta[3], len;
            dtVsub(delta, steerPos, iterPos);
            len = dtMathSqrtf(dtVdot(delta, delta));
            // If the steer target is end of path or off-mesh link, do not move past the location.
            if ((endOfPath || offMeshConnection) && len < STEP_SIZE)
                len = 1;
            else
                len = STEP_SIZE / len;
            float moveTgt[3];
            dtVmad(moveTgt, iterPos, delta, len);

            // Move
            float result[3];
            dtPolyRef visited[16];
            int nvisited = 0;
            _q->moveAlongSurface(polys[0], iterPos, moveTgt, &m_filter,
                result, visited, &nvisited, 16);

            npolys = fixupCorridor(polys, npolys, MAX_POLYS, visited, nvisited);
            npolys = fixupShortcuts(polys, npolys, _q);

            float h = 0;
            _q->getPolyHeight(polys[0], result, &h);
            result[1] = h;
            dtVcopy(iterPos, result);

            // Handle end of path and off-mesh links when close enough.
            if (endOfPath && inRange(iterPos, steerPos, SLOP, 1.0f)) {
                // Reached end of path.
                dtVcopy(iterPos, targetPos);
                if (m_nsmoothPath < MAX_SMOOTH) {
                    path.push_back(VEC3(iterPos[0], iterPos[1], iterPos[2]));
                    dtVcopy(&m_smoothPath[m_nsmoothPath * 3], iterPos);
                    m_nsmoothPath++;
                }
                break;
            }
            else if (offMeshConnection && inRange(iterPos, steerPos, SLOP, 1.0f)) {
                // Reached off-mesh connection.
                float startPos[3], endPos[3];

                // Advance the path up to and over the off-mesh connection.
                dtPolyRef prevRef = 0, polyRef = polys[0];
                int npos = 0;
                while (npos < npolys && polyRef != steerPosRef) {
                    prevRef = polyRef;
                    polyRef = polys[npos];
                    npos++;
                }
                for (int i = npos; i < npolys; ++i)
                    polys[i - npos] = polys[i];
                npolys -= npos;

                // Handle the connection.
                dtStatus status = _data->getOffMeshConnectionPolyEndPoints(prevRef, polyRef, startPos, endPos);
                if (dtStatusSucceed(status)) {
                    if (m_nsmoothPath < MAX_SMOOTH) {
                        path.push_back(VEC3(startPos[0], startPos[1], startPos[2]));
                        dtVcopy(&m_smoothPath[m_nsmoothPath * 3], startPos);
                        m_nsmoothPath++;
                        // Hack to make the dotted path not visible during off-mesh connection.
                        if (m_nsmoothPath & 1) {
                            path.push_back(VEC3(startPos[0], startPos[1], startPos[2]));
                            dtVcopy(&m_smoothPath[m_nsmoothPath * 3], startPos);
                            m_nsmoothPath++;
                        }
                    }
                    // Move position at the other side of the off-mesh link.
                    dtVcopy(iterPos, endPos);
                    float eh = 0.0f;
                    _q->getPolyHeight(polys[0], iterPos, &eh);
                    iterPos[1] = eh;
                }
            }

            // Store results.
            if (m_nsmoothPath < MAX_SMOOTH) {
                path.push_back(VEC3(iterPos[0], iterPos[1], iterPos[2]));
                dtVcopy(&m_smoothPath[m_nsmoothPath * 3], iterPos);
                m_nsmoothPath++;
            }
        }
        
    }

    /*m_npolys = 0;
    m_nsmoothPath = 0;*/
}

float CNavMeshQuery::wallDistance(VEC3 pos)
{
    m_distanceToWall = 0;

    dtStatus status = DT_SUCCESS;
    status &= _q->findNearestPoly(&pos.x, m_polyPickExt, &m_filter, &m_startRef, 0);
    assert(dtStatusSucceed(status));

    m_distanceToWall = 0.0f;
    _q->findDistanceToWall( m_startRef, &pos.x, 100.0f, &m_filter, &m_distanceToWall, m_hitPos, m_hitNormal );

    return m_distanceToWall;
}

bool CNavMeshQuery::raycast(VEC3 start, VEC3 end)
{
    m_nstraightPath = 0;

    dtStatus status = DT_SUCCESS;
    status &= _q->findNearestPoly(&start.x, m_polyPickExt, &m_filter, &m_startRef, 0);
    assert(dtStatusSucceed(status));

    float t = 0;
    m_npolys = 0;
    m_nstraightPath = 2;
    m_straightPath[ 0 ] = start.x;
    m_straightPath[ 1 ] = start.y;
    m_straightPath[ 2 ] = start.z;
    _q->raycast( m_startRef, &start.x, &end.x, &m_filter, &t, m_hitNormal, m_polys, &m_npolys, MAX_POLYS );
    if( t > 1 ) {
        // No hit
        dtVcopy( m_hitPos, &end.x );
        m_hitResult = false;
    }
    else {
        // Hit
        dtVlerp( m_hitPos, &start.x, &end.x, t );
        m_hitResult = true;
    }
    // Adjust height.
    if( m_npolys > 0 ) {
        float h = 0;
        _q->getPolyHeight( m_polys[ m_npolys - 1 ], m_hitPos, &h );
        m_hitPos[ 1 ] = h;
    }
    dtVcopy( &m_straightPath[ 3 ], m_hitPos );

    return m_hitResult;
}

VEC3 CNavMeshQuery::getRandomPoint()
{
    VEC3 randomPoint;
    dtQueryFilter filter;

    dtStatus status = _q->findRandomPoint(&filter, Random::unit, &m_startRef, &randomPoint.x);
    assert(dtStatusSucceed(status));
    return randomPoint;
}

VEC3 CNavMeshQuery::getRandomPointAroundCircle(VEC3 pos, float radius)
{
    VEC3 randomPoint;
    dtQueryFilter filter;

    dtStatus status = DT_SUCCESS;
    status &= _q->findNearestPoly(&pos.x, m_polyPickExt, &m_filter, &m_startRef, 0);
    assert(dtStatusSucceed(status));

    // Can't get the nearest poly
    if (!m_startRef)
        return pos - VEC3(0.1f);

    status &= _q->findRandomPointAroundCircle(m_startRef, &pos.x, radius, &filter, Random::unit, &m_startRef, &randomPoint.x);
    // TODO: this crashes, why? 
    // maybe m_polyPickExt is to short and then the ref is invalid?
    assert(dtStatusSucceed(status));
    return randomPoint;
}