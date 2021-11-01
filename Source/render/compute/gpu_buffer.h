#pragma once

// -------------------------------------------------------
class CGPUBuffer
{

public:

  bool create(uint32_t bytes_per_elem, uint32_t num_elems, bool is_indirect, const std::string& name = "");
  void destroy();
  void activate(int slot) const;
  void activatePS(int slot) const;
  void activateCSasUAV(int slot) const;
  void activateCSasSR(int slot) const;

  // To hold the data in the cpu
  std::vector<uint8_t>         cpu_data;

  bool copyGPUtoCPU() const;
  void copyCPUtoGPU() const;
  void copyCPUtoGPUFrom(const void* new_data) const;
  bool create(const json& j);

  ID3D11Buffer* getResource() { return buffer; }

  ID3D11Buffer*              buffer = nullptr;
  ID3D11ShaderResourceView*  srv = nullptr;
  ID3D11UnorderedAccessView* uav = nullptr;

  uint32_t                  bytes_per_elem = 0;
  uint32_t                  num_elems = 0;
  bool                      is_indirect = false;
  std::string               name;
  std::string               data_type_name;

  bool debugInMenu();
};

