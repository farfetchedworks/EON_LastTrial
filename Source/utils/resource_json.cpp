#include "mcv_platform.h"
#include "resource_json.h"

class CJsonResourceType : public CResourceType {
public:
  const char* getExtension(int idx) const { return ".json"; }
  const char* getName() const { return "Json"; }
  IResource* create(const std::string& name) const {
    CJson* res = new CJson(name);
    return res;
  }
};

// -----------------------------------------
template<>
CResourceType* getClassResourceType<CJson>() {
  static CJsonResourceType factory;
  return &factory;
}

