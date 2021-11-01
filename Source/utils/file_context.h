#pragma once

struct TFileContext {
  TFileContext(const std::string& filename) {
    files.push_back(&filename);
  }
  ~TFileContext() {
    files.pop_back();
  }
  static std::vector< const std::string* > files;
  static std::string getFileContextStack() {
    std::string txt = "";
    for (auto e : files)
      txt = "  " + (*e) + "\n" + txt;
    return txt;
  }
};

