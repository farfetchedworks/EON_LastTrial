#include "mcv_platform.h"
#include "engine.h"
#include "navmesh.h"
#include "navmesh/module_navmesh.h"
#include "render/draw_primitives.h"

class CNavMeshResourceType : public CResourceType {
public:
	const char* getExtension(int idx) const { return ".navmesh"; }
	const char* getName() const { return "NavMesh"; }
	IResource* create(const std::string& name) const {

		CFileDataProvider fdp(name.c_str());
		if (!fdp.isValid()) {
			return nullptr;
		}

		CNavMesh* navMesh = new CNavMesh();
		bool is_ok = navMesh->load(fdp);
		if (!is_ok) {
			delete navMesh;
			return nullptr;
		}
		navMesh->setResourceName(name);
		return navMesh;
	}
};

template<>
CResourceType* getClassResourceType<CNavMesh>() {
	static CNavMeshResourceType factory;
	return &factory;
}

// ----------------------------------------------------------------

static const int NAVMESHSET_MAGIC = 'M' << 24 | 'S' << 16 | 'E' << 8 | 'T'; //'MSET';
static const int NAVMESHSET_VERSION = 1;

struct NavMeshSetHeader
{
	int magic;
	int version;
	int numTiles;
	dtNavMeshParams params;
};

struct NavMeshTileHeader
{
	dtTileRef tileRef;
	int dataSize;
};

struct NavMeshVertex {
	VEC3 pos = {};
	VEC4 col = {};
};

CNavMesh::~CNavMesh()
{
	dtFreeNavMesh(m_navMesh);

	m_debugMesh->destroy();
	m_debugBoundsMesh->destroy();

	delete m_debugMesh;
	delete m_debugBoundsMesh;
}

bool CNavMesh::load(CFileDataProvider& dp)
{
	// Load NavMesh from file
	m_navMesh = parse(dp);
	if (!m_navMesh)
		return false;

	// Generate debug Mesh
	std::vector<NavMeshVertex> vertices = {};
	fillDebugMesh(vertices);
	m_debugMesh = new CMesh();
	bool is_ok = m_debugMesh->create(vertices.data(), (uint32_t)vertices.size() * sizeof(NavMeshVertex), sizeof(NavMeshVertex), nullptr,
		0, 0, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, getVertexDeclarationByName("PosColor"));
	assert(is_ok);

	// Generate debug bounds Mesh
	vertices.clear();
	fillDebugBoundsMesh(vertices);
	m_debugBoundsMesh = new CMesh();
	is_ok = m_debugBoundsMesh->create(vertices.data(), (uint32_t)vertices.size() * sizeof(NavMeshVertex), sizeof(NavMeshVertex), nullptr,
		0, 0, D3D11_PRIMITIVE_TOPOLOGY_LINELIST, getVertexDeclarationByName("PosColor"));
	assert(is_ok);

	// Initialize naveMesh Query
	m_navQuery.init(m_navMesh);
	
	// Do some queries to test the NavMesh
	// test();

	name = dp.getName();

	return true;
}

dtNavMesh* CNavMesh::parse(CFileDataProvider& dp)
{
	NavMeshSetHeader header;
	dp.read(header);
	
	if (header.magic != NAVMESHSET_MAGIC) return nullptr;
	if (header.version != NAVMESHSET_VERSION) return nullptr;

	dtNavMesh* mesh = dtAllocNavMesh();
	if (!mesh) return nullptr;
	
	dtStatus status = mesh->init(&header.params);
	if (dtStatusFailed(status)) return nullptr;

	// Read tiles.
	for (int i = 0; i < header.numTiles; ++i)
	{
		NavMeshTileHeader tileHeader;
		dp.read(tileHeader);

		if (!tileHeader.tileRef || !tileHeader.dataSize)
			break;

		unsigned char* data = (unsigned char*)dtAlloc(tileHeader.dataSize, DT_ALLOC_PERM);
		if (!data) break;
		memset(data, 0, tileHeader.dataSize);
		dp.readBytes(tileHeader.dataSize, data);

		mesh->addTile(data, tileHeader.dataSize, DT_TILE_FREE_DATA, tileHeader.tileRef, 0);
	}

	return mesh;
}

void CNavMesh::update(float dt)
{
	m_navQuery.update(dt);
}

void CNavMesh::renderInMenu()
{
	ImGui::Text("%s", getName().c_str());
	ImGui::Checkbox("Render debug", &m_debugEnabled);
}

void CNavMesh::renderDebug()
{
	if (!m_debugEnabled) return;

	// DrawNavmesh
	drawPrimitive(m_debugMesh, MAT44::Identity, Colors::White, RSConfig::CULL_NONE);
	drawPrimitive(m_debugBoundsMesh, MAT44::Identity, Colors::White, RSConfig::WIREFRAME);

	// NavQuery
	m_navQuery.render();
}

void CNavMesh::fillDebugMesh(std::vector<NavMeshVertex>& vertices)
{
	for (int t = 0; t < m_navMesh->getMaxTiles(); ++t) {

		const dtMeshTile* tile = ((const dtNavMesh*)m_navMesh)->getTile(t);
		if (!tile->header)
			continue;

		for (int i = 0; i < tile->header->polyCount; ++i)
		{
			const dtPoly* p = &tile->polys[i];
			if (p->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)	// Skip off-mesh links.
				continue;

			VEC4 color = areaToColor(p->getArea());
			const dtPolyDetail* pd = &tile->detailMeshes[i];

			for (int j = 0; j < pd->triCount; ++j)
			{
				const unsigned char* t = &tile->detailTris[(pd->triBase + j) * 4];
				for (int k = 0; k < 3; ++k)
				{
					if (t[k] < p->vertCount)
						vertex(vertices, &tile->verts[p->verts[t[k]] * 3], color);
					else
						vertex(vertices , &tile->detailVerts[(pd->vertBase + t[k] - p->vertCount) * 3], color);
				}
			}
		}
	}
}

void CNavMesh::fillDebugBoundsMesh(std::vector<NavMeshVertex>& vertices)
{
	for (int t = 0; t < m_navMesh->getMaxTiles(); ++t) {

		const dtMeshTile* tile = ((const dtNavMesh*)m_navMesh)->getTile(t);
		if (!tile->header)
			continue;

		// Draw inter poly boundaries
		drawPolyBoundaries(vertices, tile, Colors::White, true);

		// Draw outer poly boundaries
		drawPolyBoundaries(vertices, tile, Colors::Black, false);
	}
}

static float distancePtLine2d(const float* pt, const float* p, const float* q)
{
	float pqx = q[0] - p[0];
	float pqz = q[2] - p[2];
	float dx = pt[0] - p[0];
	float dz = pt[2] - p[2];
	float d = pqx * pqx + pqz * pqz;
	float t = pqx * dx + pqz * dz;
	if (d != 0) t /= d;
	dx = p[0] + t * pqx - pt[0];
	dz = p[2] + t * pqz - pt[2];
	return dx * dx + dz * dz;
}

void CNavMesh::drawPolyBoundaries(std::vector<NavMeshVertex>& vertices, const dtMeshTile* tile, 
	VEC4 color, bool inner)
{
	static const float thr = 0.01f * 0.01f;

	for (int i = 0; i < tile->header->polyCount; ++i)
	{
		const dtPoly* p = &tile->polys[i];

		if (p->getType() == DT_POLYTYPE_OFFMESH_CONNECTION) continue;

		const dtPolyDetail* pd = &tile->detailMeshes[i];

		for (int j = 0, nj = (int)p->vertCount; j < nj; ++j)
		{
			VEC4 c = color;
			if (inner)
			{
				if (p->neis[j] == 0) continue;
				if (p->neis[j] & DT_EXT_LINK)
				{
					bool con = false;
					for (unsigned int k = p->firstLink; k != DT_NULL_LINK; k = tile->links[k].next)
					{
						if (tile->links[k].edge == j)
						{
							con = true;
							break;
						}
					}
					if (con)
						continue;
						// c = Colors::Blue;
					else
						c = Colors::Red;
				}
				else
					continue;
					// c = VEC4(0, 48, 64, 32);
			}
			else
			{
				if (p->neis[j] != 0) continue;
			}

			const float* v0 = &tile->verts[p->verts[j] * 3];
			const float* v1 = &tile->verts[p->verts[(j + 1) % nj] * 3];

			// Draw detail mesh edges which align with the actual poly edge.
			// This is really slow.
			for (int k = 0; k < pd->triCount; ++k)
			{
				const unsigned char* t = &tile->detailTris[(pd->triBase + k) * 4];
				const float* tv[3];
				for (int m = 0; m < 3; ++m)
				{
					if (t[m] < p->vertCount)
						tv[m] = &tile->verts[p->verts[t[m]] * 3];
					else
						tv[m] = &tile->detailVerts[(pd->vertBase + (t[m] - p->vertCount)) * 3];
				}
				for (int m = 0, n = 2; m < 3; n = m++)
				{
					if ((dtGetDetailTriEdgeFlags(t[3], n) & DT_DETAIL_EDGE_BOUNDARY) == 0)
						continue;

					if (distancePtLine2d(tv[n], v0, v1) < thr &&
						distancePtLine2d(tv[m], v0, v1) < thr)
					{
						vertex(vertices, tv[n], c);
						vertex(vertices, tv[m], c);
					}
				}
			}
		}
	}
}

void CNavMesh::vertex(std::vector<NavMeshVertex>& vertices, const float* p, VEC4 color)
{
	vertices.push_back({VEC3(p[0], p[1], p[2]), color});
}

VEC4 CNavMesh::areaToColor(unsigned int area)
{
	switch (area)
	{
	case SAMPLE_POLYAREA_GROUND: return Colors::PalidGreen;
		// Water : blue
	case SAMPLE_POLYAREA_WATER: return VEC4(0, 0, 255, 255);
		// Road : brown
	case SAMPLE_POLYAREA_ROAD: return VEC4(50, 20, 12, 255);
		// Door : cyan
	case SAMPLE_POLYAREA_DOOR: return VEC4(0, 255, 255, 255);
		// Grass : green
	case SAMPLE_POLYAREA_GRASS: return VEC4(0, 255, 0, 255);
		// Jump : yellow
	case SAMPLE_POLYAREA_JUMP: return VEC4(255, 255, 0, 255);
		// Unexpected : red
	default: return VEC4(255, 0, 0, 255);
	}
}
