#include "mcv_platform.h"
#include "utils/data_provider.h"
#include "mesh_io.h"

bool TMeshIO::read(IDataProvider& dp) {

  assert(dp.isValid() || fatal("Invalid data provider while reading mesh"));

  // Read the header
  dp.read(header);
  assert(header.isValid());

  bool end_of_file_found = false;
  while (!end_of_file_found) {

    Riff riff;
    dp.read(riff);

    switch (riff.magic) {

    case magic_vertices:
      assert(riff.bytes == header.bytes_per_vertex * header.num_vertices);
      vertices.resize(riff.bytes);
      dp.readBytes(riff.bytes, vertices.data());
      break;

    case magic_indices:
      assert(riff.bytes == header.bytes_per_index * header.num_indices);
      indices.resize(riff.bytes);
      dp.readBytes(riff.bytes, indices.data());
      break;

    case magic_collision_cooked:
      assert(riff.bytes > 0);
      collision_cooked.resize(riff.bytes);
      dp.readBytes(riff.bytes, collision_cooked.data());
      break;

    case magic_mesh_group:
      assert(riff.bytes > 0);
      assert(riff.bytes % sizeof(TMeshGroup) == 0);
      mesh_groups.resize(riff.bytes / sizeof(TMeshGroup));
      dp.readBytes(riff.bytes, mesh_groups.data());
      break;

    case magic_end_of_file:
      end_of_file_found = true;
      break;

    default:
      fatal("Invalid magic %08x found while reading mesh file %s\n", riff.magic, dp.getName());
    }
  }

  // Create a default mesh group in case none has been read
  if (mesh_groups.empty()) {
    TMeshGroup mg;
    mg.first_index = 0;
    mg.num_indices = (uint32_t)indices.size() / header.bytes_per_index;
    mesh_groups.push_back(mg);
  }

  return true;
}

static void writeBuffer(IDataSaver& ds, uint32_t magic, const void* data, size_t data_size) {
  TMeshIO::Riff riff;
  riff.bytes = (uint32_t)data_size;
  riff.magic = magic;
  ds.write(riff);
  if (data_size > 0) {
    assert(data);
    ds.writeBytes(data_size, data);
  }
}

bool TMeshIO::write(IDataSaver& ds) {
  ds.write(header);
  writeBuffer(ds, magic_vertices, vertices.data(), vertices.size());
  writeBuffer(ds, magic_indices, indices.data(), indices.size());
  if (collision_cooked.size())
    writeBuffer(ds, magic_collision_cooked, collision_cooked.data(), collision_cooked.size());
  if(mesh_groups.size())
    writeBuffer(ds, magic_mesh_group, mesh_groups.data(), mesh_groups.size() * sizeof( TMeshGroup ));
  writeBuffer(ds, magic_end_of_file, nullptr, 0);

  return true;
}
