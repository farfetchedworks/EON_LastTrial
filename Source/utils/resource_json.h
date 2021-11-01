#pragma once

#include "resources/resources.h"

class CJson : public IResource {
  json j;
public:
  CJson(const std::string& filename) {
    // Do the real parsing of the json
    j = loadJson(filename);
  }

  bool renderInMenu() const override {
    ImGui::Text(j.dump(2, ' ').c_str());
    return false;
  }

  const json& getJson() const { return j; }

  void onFileChange(const std::string& filename) override { 
    // Reload the json if the file matches
    if (getName() == filename)
      j = loadJson(filename);
  }

};
