#include "mcv_platform.h"
#include "handle/handle.h"
#include "comp_name.h"
#include "comp_transform.h"
#include "render/draw_primitives.h"

DECL_OBJ_MANAGER("name", TCompName)

std::unordered_map< std::string, CHandle > TCompName::all_names;
std::unordered_map< std::string, VHandles > TCompName::all_prefabs;

void TCompName::setName(const char* new_name) {
  strcpy(name, new_name);

  // Store the handle of the CompName, not the Entity, 
  // because during load, I still don't have an owner
  all_names[name] = CHandle(this);
}

void TCompName::debugInMenu() {
  ImGui::Text(name);

  if (ImGui::TreeNode("All Names"))
  {
      for (auto n : TCompName::all_names) {
          ImGui::Text(n.first.c_str());
      }
      ImGui::TreePop();
  }

  if (ImGui::TreeNode("All Prefabs"))
  {
      for (auto n : TCompName::all_prefabs) {
          if (ImGui::TreeNode(n.first.c_str()))
          {
              for (auto h : n.second) {
                  h.debugInMenu();
              }
              ImGui::TreePop();
          }
      }
      ImGui::TreePop();
  }
}

void TCompName::load(const json& j, TEntityParseContext& ctx) {
  assert(j.is_string());
  std::string name = j.get<std::string>();

  // Don't spawn things with same name
  if (getEntityByName(name).isValid()) {
      name += std::to_string(Random::unit());
  }

  setName(name.c_str());
}

bool isAlive(const std::string& name) {
    return getEntityByName(name).isValid();
}

CHandle getEntityByName(const std::string& name) {

  auto it = TCompName::all_names.find(name);
  if (it == TCompName::all_names.end())
    return CHandle();

  // We are storing the handle of the component, but we want to return
  // the handle of the entity owner of that component
  CHandle h_name = it->second;
  return h_name.getOwner();
}

VHandles getEntitiesByName(const std::string& name) {

    VHandles hs;

    for (auto n : TCompName::all_names) {

        CEntity* e = n.second.getOwner();
        if (!e)
            continue;

        if(name == n.first)
            hs.push_back(e);
    }

    return hs;
}

VHandles getEntitiesByString(const std::string& token, std::function<bool(CHandle)> fn) {

    VHandles v;

    for (auto n : TCompName::all_names) {

        CEntity* e = n.second.getOwner();
        if (!e)
            continue;

        std::string name = n.first;
        std::string s_token = token;

        toLowerCase(name);
        toLowerCase(s_token);

        if (!includes(name, s_token))
            continue;

        if (fn && !fn(e))
            continue;

        v.push_back(e);
    }

    return v;
}

void TCompName::renderDebug() {

#ifdef _DEBUG
    if (!strcmp(name, "camera")) {
        return;
    }

    TCompTransform* trans = get<TCompTransform>();
    if (!trans)
        return;
    VEC3 pos = trans->getPosition();
    drawText3D(pos, Colors::White, name);
#endif

}