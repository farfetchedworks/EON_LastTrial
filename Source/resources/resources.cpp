#include "mcv_platform.h"
#include "resources.h"

CResourcesManager Resources;

const IResource* CResourcesManager::get(const std::string& name) {

  // Search the resource by name
  auto it = all_resources.find(name);

  // If found, return it
  if (it != all_resources.end()) 
    return it->second;

  // The resource is not registered... we need to load it 

  // Search for the extension to identify the type of resource
  auto idx = name.find_last_of(".");
  if (idx == std::string::npos) {
    fatal("Can't identify resource type from resource name '%s'\n", name.c_str());
    return nullptr;
  }

  // Get the extension from the name: .mesh
  std::string extension = name.substr(idx);
  auto it_type = all_resource_types.find(extension);
  if (it_type == all_resource_types.end()) {
    fatal("Don't know how to create resources with extension '%s' for resource %s\n", extension.c_str(), name.c_str());
    return nullptr;
  }

  CResourceType* res_type = it_type->second;
  assert(res_type);

  // Use the factory to do the real creation of the resource
  PROFILE_FUNCTION(name);
  IResource* new_resource = res_type->create(name);
  if (new_resource == nullptr) {
    fatal("Failed to create resource '%s'\n", name.c_str());
    return nullptr;
  }

  // If someone else request the same resource, return it
  registerResource(new_resource, name, res_type);

  return new_resource;
}

bool CResourcesManager::renderInMenu() {
  bool changed = false;

  if (ImGui::TreeNode("Resources...")) {

    ImGui::InputText("Search", buff, 128);
    ImGui::Separator();

    for (auto res_type_it : all_resource_types) {
      const CResourceType* res_type = res_type_it.second;

      // If a resource class has several extensions, only show the first one
      if (res_type->getNumExtensions() > 1 && res_type->getExtension(0) != res_type_it.first)
        continue;

      // Filter by type...
      if (ImGui::TreeNode(res_type->getName())) {

        // Scan ALL resources
        for (auto it : all_resources) {
          IResource* res = it.second;
          assert(res);

          // Show it if it's the type we are showing noe
          if (res->getResourceType() == res_type) {

            std::string buffString = buff;
            std::string name = it.first.c_str();

            toLowerCase(buffString);
            toLowerCase(name);

            if (name.size() > 0 && name.find(buffString) == std::string::npos)
                continue;

            if (ImGui::TreeNode(it.first.c_str())) {
              
              // ... Show the details of the resource
              changed |= res->renderInMenu();

              ImGui::TreePop();
            }
          }
        }
        ImGui::TreePop();
      }

    }
    ImGui::TreePop();
  }

  return changed;
}


bool CResourcesManager::editResourceType(
  const IResource** resource_to_edit
, const CResourceType* res_type
) const {
  assert(resource_to_edit);
  const IResource* user_res = *resource_to_edit;
  assert(user_res->getResourceType() == res_type);
  bool changed = false;

  if (ImGui::TreeNode(user_res->getName().c_str())) {

    // Scan ALL resources
    for (auto it : all_resources) {
      IResource* new_res = it.second;

      // Show it if it's the type we are showing noe
      if (new_res->getResourceType() == res_type) {

        if (ImGui::SmallButton(new_res->getName().c_str())) {
          *resource_to_edit = new_res;
          changed = true;
          break;
        }
      }
    }

    ImGui::TreePop();
  }

  return changed;
}

void CResourcesManager::onFileChanged(const std::string& filename) {
  for (auto it : all_resources)
    it.second->onFileChange(filename);
}

bool CResourcesManager::destroyResource(const std::string& name) {
    
    // Search the resource by name
    auto it = all_resources.find(name);

    // If found, return it
    if (it != all_resources.end()) {
        all_resources[name]->destroy();
        all_resources.erase(name);
        return true;
    }

    return false;
}

void CResourcesManager::destroy() {
  for (auto it : all_resources) {
    IResource* res = it.second;
    assert(res);
    res->destroy();
    delete res;
  }
  all_resources.clear();
}
