#pragma once

#include "data_saver.h"
#include "data_provider.h"

struct TBuffer : public std::vector<uint8_t> {
  TBuffer(size_t nbytes) {
    resize(nbytes);
  }
  TBuffer() = default;
  void saveRaw(const std::string& file) const {
    CFileDataSaver fds(file.c_str());
    fds.writeBytes(size(), data());
  }
  bool loadRaw(const std::string& file) {
    struct stat result;
    if (stat(file.c_str(), &result))
      return false;
    resize(result.st_size);
    CFileDataProvider fdp(file.c_str());
    fdp.readBytes(size(), data());
    return true;
  }
};

