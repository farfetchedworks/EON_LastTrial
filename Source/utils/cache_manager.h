#pragma once

#include "utils/buffer.h"

class CCacheManager {

  std::string title;
  std::string root_folder;

  std::string storeFilename(const char* name) const;
  void storeEntry(const TBuffer& cd) const;

  struct TMetaData {
    static const uint32_t meta_data_magic = 0x81828384;
    uint32_t magic = meta_data_magic;
    uint32_t orig_filename_id = 0;
    uint64_t data_size = 0;
    int64_t st_mtime = 0;
    bool isValid() const { return magic == meta_data_magic; }
  };

public:

  CCacheManager(const char* new_title, const char* root_folder);

  bool isCached(const std::string& orig_name, const std::string& cache_name, TBuffer& buffer);
  void cache(const std::string& orig_name, const std::string& cache_name, const TBuffer& buffer);
  void debugInMenu();

  uint32_t nmisses = 0;
  uint32_t nhits = 0;
  uint32_t nsaved = 0;
  uint32_t nerrors = 0;
};


