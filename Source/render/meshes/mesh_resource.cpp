#include "mcv_platform.h"
#include "mesh_io.h"

class CMeshResourceType : public CResourceType {

  bool loadMesh(CMesh* mesh, const std::string& name) const
  {
    TMeshIO mesh_io;
    CFileDataProvider fdp(name.c_str());
    bool is_ok = mesh_io.read(fdp);
    if (!is_ok)
      return false;

    is_ok = mesh->create(mesh_io);
    assert(is_ok);
    return is_ok;
  }

public:
  const char* getExtension(int idx) const { return ".mesh"; }

  const char* getName() const { return "Mesh"; }
  IResource* create(const std::string& name) const {
    TFileContext file_context(name);

    // Standard path
    CMesh* mesh = new CMesh();
    bool is_ok = loadMesh(mesh, name);
    return mesh;
  }
};

// -----------------------------------------
template<> 
CResourceType* getClassResourceType<CMesh>() {
  static CMeshResourceType factory;
  return &factory;
}

