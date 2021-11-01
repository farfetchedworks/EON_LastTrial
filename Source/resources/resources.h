#pragma once

// CMesh -> .mesh
// PipelineState
// Json
// 

class IResource;

// -----------------------------------------
class CResourceType {
public:
  virtual const char* getExtension(int idx = 0) const = 0;
  virtual int getNumExtensions() const { return 1; }
  virtual const char* getName() const = 0;
  virtual IResource* create(const std::string& name) const = 0;
};

// -----------------------------------------
template< typename T >
CResourceType* getClassResourceType();

// -----------------------------------------
class IResource {
protected:
  std::string          name;     // full path 
  const CResourceType* resource_type = nullptr;

public:
  virtual ~IResource() {}

  const std::string& getName() const { return name; }
  const CResourceType* getResourceType() const {
    return resource_type;
  }

  void setResourceType(const CResourceType* new_resource_type) {
    resource_type = new_resource_type;
  }
  virtual void setResourceName(const std::string& new_name) {
    name = new_name;
  }

  virtual void destroy() {}
  virtual bool renderInMenu() const { return false; }
  virtual void onFileChange(const std::string& filename) { }

  template<typename TargetType>
  const TargetType* as() const {
    assert(resource_type != nullptr);
    const CResourceType* target_type = getClassResourceType<TargetType>();
    assert(target_type == resource_type || fatal(
      "Can't cast resource %s of type %s to target type %s\n"
      , getName().c_str()
      , resource_type->getName()
      , target_type->getName()
      )
    );
    return static_cast<const TargetType*>(this);
  }

};

// -----------------------------------------
class CResourcesManager {

  char buff[128];

  std::map< std::string, IResource* > all_resources;

  // Maps extensions to resource type
  std::map< std::string, CResourceType* > all_resource_types;

public:

  bool exists(const std::string& name) const {
    return all_resources.find(name) != all_resources.end();
  }

  const IResource* get(const std::string& name);

  void registerResourceType(CResourceType* new_resource_type) {
    assert(new_resource_type);
    // A CResourceType class can be associated to more than
    // one resource type/extension
    for (int i = 0; i < new_resource_type->getNumExtensions(); ++i)
    {
      const char* extension = new_resource_type->getExtension(i);
      // Extension can't be duped
      assert(all_resource_types.find(extension) == all_resource_types.end());

      // Associate .mesh with the resource_type/factory
      all_resource_types[extension] = new_resource_type;
    }
  }

  void registerResource(IResource* new_resource, const std::string& new_name, const CResourceType* new_type) {
    assert(new_resource);
    assert(!new_name.empty());
    assert(new_type);
    new_resource->setResourceType(new_type);
    new_resource->setResourceName(new_name);
    assert(all_resources.find(new_name) == all_resources.end());
    all_resources[new_name] = new_resource;
  }

  bool renderInMenu();

  // Deduce the class of resource to edit and forward it to the next non-template fn
  template< typename T >
  bool edit(const T** resource_to_edit) const {
    const CResourceType* res_type = getClassResourceType<T>();
    return editResourceType((const IResource**)resource_to_edit, res_type);
  }

  bool editResourceType(const IResource** resource_to_edit, const CResourceType* res_type ) const;

  void onFileChanged(const std::string& strfilename);
  bool destroyResource(const std::string& name);
  void destroy();
};

extern CResourcesManager Resources;








