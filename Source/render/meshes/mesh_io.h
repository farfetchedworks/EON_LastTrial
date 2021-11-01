#pragma once

#include "mesh_group.h"

class IDataProvider;

struct TMeshIO {
  
  static constexpr size_t magic_header = 0x33888800;
  static constexpr size_t magic_end_of_header = 0x33888801;
  static constexpr size_t magic_vertices = 0x33990011;
  static constexpr size_t magic_indices = 0x33990022;
  static constexpr size_t magic_mesh_group = 0x33990033;
  static constexpr size_t magic_collision_cooked = 0x33991133;
  static constexpr size_t magic_end_of_file = 0x33888888;
  
  struct Riff {
    uint32_t magic = 0;
    uint32_t bytes = 0;
  };

  struct THeader {

    uint32_t header = 0;
    uint32_t version_number = 0;
    uint32_t primitive_type = 0;
    uint32_t num_primitives = 0;

    uint32_t num_vertices = 0;
    uint32_t num_indices = 0;
    uint32_t bytes_per_vertex = 0;
    uint32_t bytes_per_index = 0;

    static constexpr uint32_t current_version = 1;
    static constexpr size_t vertex_type_name_length = 32;
    char     vertex_type_name[vertex_type_name_length] = {};

    uint32_t end_of_header = 0;

    bool isValid() const {
        return header == magic_header
        && end_of_header == magic_end_of_header
        && version_number == current_version
        ;
    }

  };

  using TBuffer = std::vector<uint8_t>;

  THeader header;
  TBuffer vertices;
  TBuffer indices;
  TBuffer collision_cooked;
  VMeshGroups mesh_groups;

  bool read(IDataProvider& dp);
  bool write(IDataSaver& ds);

};

