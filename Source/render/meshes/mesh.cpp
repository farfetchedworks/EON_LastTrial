#include "mcv_platform.h"
#include "mesh_io.h"

bool CMesh::create(TMeshIO& mesh_io) {
  return create(mesh_io.vertices.data()
    , mesh_io.vertices.size()
    , mesh_io.header.bytes_per_vertex

    , mesh_io.indices.data()
    , mesh_io.indices.size()
    , mesh_io.header.bytes_per_index
    , (D3D_PRIMITIVE_TOPOLOGY)mesh_io.header.primitive_type
    , getVertexDeclarationByName(mesh_io.header.vertex_type_name)
    , &mesh_io.mesh_groups
  );
}

void CMesh::setResourceName(const std::string& new_name) {
  IResource::setResourceName(new_name);
  if (vb)
    setDXName(vb, new_name.c_str());
  if (ib)
    setDXName(ib, new_name.c_str());
}

bool CMesh::create(
  const void* vertex_data,
  size_t bytes_vertex_data, 
  uint32_t new_bytes_per_vertex, 
  const void* new_index_data,
  size_t new_bytes_index_data,
  uint32_t new_bytes_per_index, 
  D3D11_PRIMITIVE_TOPOLOGY new_topology,
  const CVertexDeclaration* new_vertex_decl,
  const VMeshGroups* new_mesh_groups
  ) {

  // Do some checks
  assert(vertex_data != nullptr);
  assert(bytes_vertex_data > 0);
  assert(new_bytes_per_vertex > 0);
  assert(vb == nullptr);

  topology = new_topology;
  vertex_decl = new_vertex_decl;
  assert(vertex_decl);

  // Create vertex buffer
  bytes_per_vertex = new_bytes_per_vertex;
  // The buffer es not contains a partial number of vertices
  assert(bytes_vertex_data % new_bytes_per_vertex == 0);
  num_vertices = (uint32_t)bytes_vertex_data / new_bytes_per_vertex;

  D3D11_BUFFER_DESC bd = {};
  bd.Usage = D3D11_USAGE_DEFAULT;
  bd.ByteWidth = (uint32_t)bytes_vertex_data;
  bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
  bd.CPUAccessFlags = 0;

  D3D11_SUBRESOURCE_DATA InitData = {};
  InitData.pSysMem = vertex_data;
  HRESULT hr = Render.device->CreateBuffer(&bd, &InitData, &vb);
  assert(!FAILED(hr));

  // Compute aabb
  AABB::CreateFromPoints(aabb, num_vertices, (const VEC3*)vertex_data, bytes_per_vertex);

  if (new_index_data) {
    assert(new_bytes_index_data > 0);
    assert(new_bytes_per_index == 2 || new_bytes_per_index == 4);

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = (uint32_t)new_bytes_index_data;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;
    InitData.pSysMem = new_index_data;
    hr = Render.device->CreateBuffer(&bd, &InitData, &ib);
    assert(!FAILED(hr));

    // Save values 
    assert(new_bytes_index_data % new_bytes_per_index == 0);      // No partial indices
    num_indices = (uint32_t)new_bytes_index_data / new_bytes_per_index;
    bytes_per_index = new_bytes_per_index;
  }

  // Save the information
  if (new_mesh_groups) {
    mesh_groups = *new_mesh_groups;
    assert(mesh_groups.size() > 0);
  }
  else {
    mesh_groups.resize(1);
    mesh_groups[0].first_index = 0;
    mesh_groups[0].num_indices = ib ? num_indices : num_vertices;
  }

  return true;
}

void CMesh::destroy() {
  SAFE_RELEASE(vb);
  SAFE_RELEASE(ib);
}

void CMesh::activate() const {
  assert(vb);
  // Set vertex buffer
  UINT offset = 0;

  Render.ctx->IASetVertexBuffers(0, 1, &vb, &bytes_per_vertex, &offset);
  if (ib) 
    Render.ctx->IASetIndexBuffer(ib, bytes_per_index == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
  Render.ctx->IASetPrimitiveTopology(topology);
}

void CMesh::render() const {
  if (ib) {
    assert(num_indices > 0);
    Render.ctx->DrawIndexed(num_indices, 0, 0);
  }
  else {
    assert(num_vertices > 0);
    Render.ctx->Draw(num_vertices, 0);
  }
}

void CMesh::renderGroup(uint32_t group_idx) const {
  assert(group_idx < mesh_groups.size());

  const TMeshGroup& g = mesh_groups[group_idx];
  if (ib) {
    assert(g.num_indices > 0);
    Render.ctx->DrawIndexed(g.num_indices, g.first_index, 0);
  }
  else {
    assert(num_vertices > 0);
    Render.ctx->Draw(g.num_indices, g.first_index);
  }
}

void CMesh::renderGroupInstanced(uint32_t group_idx, uint32_t num_instances) const
{
  assert(group_idx < mesh_groups.size());

  const TMeshGroup& g = mesh_groups[group_idx];
  if (ib)
  {
    assert(g.num_indices > 0);
    Render.ctx->DrawIndexedInstanced(g.num_indices, num_instances, g.first_index, 0, 0);
  }
  else
  {
    assert(num_vertices > 0);
    Render.ctx->DrawInstanced(g.num_indices, num_instances, g.first_index, 0);
  }
}


bool CMesh::renderInMenu() const {
  assert(vertex_decl);
  int total_bytes = ( bytes_per_vertex * num_vertices + 1023 ) >> 10;   // Divide by 1024
  ImGui::Text("%d Vertices %s (%d bytes per vtx ~= %dK)", num_vertices, vertex_decl->name.c_str(), bytes_per_vertex, total_bytes );
  ImGui::Text("%d Indices", num_indices );
  return false;
}
