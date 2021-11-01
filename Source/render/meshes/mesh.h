#pragma once

#include <cstdint>
#include "resources/resources.h"
#include "mesh_group.h"

struct TMeshIO;

class CMesh : public IResource {

protected:

  ID3D11Buffer* vb = nullptr;
  ID3D11Buffer* ib = nullptr;
  uint32_t num_vertices = 0;
  uint32_t bytes_per_vertex = 0;
  uint32_t num_indices = 0;
  uint32_t bytes_per_index = 0;
  D3D11_PRIMITIVE_TOPOLOGY topology;
  const CVertexDeclaration* vertex_decl = nullptr;
  VMeshGroups mesh_groups;
  AABB        aabb;

public:

  bool create(TMeshIO& mesh_io);

  bool create(
    const void* vertex_data,
    size_t bytes_vertex_data, 
    uint32_t new_bytes_per_vertex,
    const void* index_data = nullptr,
    size_t bytes_index_data = 0,
    uint32_t bytes_per_index = 0, 
    D3D11_PRIMITIVE_TOPOLOGY new_topology = D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
    const CVertexDeclaration* new_vertex_decl = getVertexDeclarationByName("PosColor"),
    const VMeshGroups* mesh_groups = nullptr
  );
  void destroy() override;

  void setResourceName(const std::string& new_name) override;

  void render() const;
  void activate() const;

  void renderGroup(uint32_t group_idx) const;
  void renderGroupInstanced(uint32_t group_idx, uint32_t num_instances) const;

  bool renderInMenu() const override;

  const AABB& getAABB() const { return aabb; }
  const VMeshGroups& getGroups() const { return mesh_groups; }
  const CVertexDeclaration* getVertexDecl() const { return vertex_decl; }

};


