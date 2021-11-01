#pragma once

class IDataProvider {
public:
  virtual ~IDataProvider() { }
  virtual bool isValid() const = 0;
  virtual void readBytes(size_t bytes, void* out_buffer) = 0;
  virtual const char* getName() const = 0;

  template< typename T>
  void read(T& obj) {
    readBytes(sizeof(T), &obj);
  }
};

class CFileDataProvider : public IDataProvider {
  FILE* f = nullptr;
  const char* filename = nullptr;
public:
  CFileDataProvider(const char* new_filename)
    : filename(new_filename) {
    f = fopen(filename, "rb");
  }
  ~CFileDataProvider() {
    close();
  }
  bool close() {
    if (!f)
      return false;
    fclose(f);
    f = nullptr;
    return true;
  };
  bool isValid() const override {
    return f != nullptr;
  };
  void readBytes(size_t bytes_to_read, void* out_buffer) override {
    auto bytes_read = fread(out_buffer, 1, bytes_to_read, f);
    assert(bytes_read == bytes_to_read);
  }
  const char* getName() const {
    return filename;
  }
};

class CMemoryDataProvider : public IDataProvider, public std::vector<uint8_t> {
  size_t offset = 0;
  size_t total_size = 0;
  const char* filename = nullptr;
public:

  CMemoryDataProvider(const char* new_filename)
    : filename(new_filename) {
    
    // Read the full file in memory
    FILE* f = fopen(filename, "rb");
    assert(f);
    // Go to the end of the file 
    fseek(f, 0, SEEK_END);
    // Ask the current position, so now we know the file size
    total_size = ftell(f);
    // Return to the start of the file
    fseek(f, 0, SEEK_SET);

    resize(total_size);
    auto bytes_read = fread(data(), 1, total_size, f);
    assert(bytes_read == total_size);
    fclose(f);
  }

  bool isValid() const override {
    return !empty();
  }

  bool eof() const {
    return offset == total_size;
  }

  size_t remainingBytes() const {
    return total_size - offset;
  }

  uint8_t* nextData() {
    return data() + offset;
  }

  void readBytes(size_t bytes_to_read, void* out_buffer) override {
    assert(offset + bytes_to_read <= total_size);
    memcpy(out_buffer, nextData(), bytes_to_read);
    offset += bytes_to_read;
  }

  const char* getName() const {
    return filename;
  }
};

