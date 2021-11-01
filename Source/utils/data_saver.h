#pragma once

class IDataSaver {
public:
  virtual ~IDataSaver() { }
  virtual bool isValid() const = 0;
  virtual void writeBytes(size_t bytes, const void* data_to_save) = 0;
  virtual const char* getName() const = 0;

  template< typename T>
  void write(T& obj) {
    writeBytes(sizeof(T), &obj);
  }
};

class CFileDataSaver : public IDataSaver {
  FILE* f = nullptr;
  const char* filename = nullptr;
public:
  CFileDataSaver(const char* new_filename)
    : filename(new_filename) {
    f = fopen(filename, "wb");
  }
  ~CFileDataSaver() {
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
  void writeBytes(size_t bytes_to_write, const void* data_to_save) override {
    auto bytes_read = fwrite(data_to_save, 1, bytes_to_write, f);
    assert(bytes_read == bytes_to_write);
  }
  const char* getName() const {
    return filename;
  }
};

class CMemoryDataSaver : public IDataSaver
{
public:
  // Container
  std::vector<uint8_t> buffer;
  bool isValid() const { return !buffer.empty(); }
  void writeBytes(size_t bytes, const void* data_to_save)
  {
    size_t curr_size = buffer.size();
    buffer.resize(curr_size + bytes );
    memcpy( buffer.data() + curr_size, data_to_save, bytes );
  }
  virtual const char* getName() const { return "mds"; }
};