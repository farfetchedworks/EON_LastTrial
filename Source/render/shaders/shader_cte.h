#pragma once
#include "engine.h"

class CBaseShaderCte {

protected:

  ID3D11Buffer* cte = nullptr;
  int slot = -1;
  std::string name;
  uint32_t    num_bytes = 0;
  
public:

  void updateFromCPU(const void* new_cpu_data) {
    if (CEngine::get().getMainThreadId() != std::this_thread::get_id())
        return;
    assert(cte);
    Render.ctx->UpdateSubresource(cte, 0, nullptr, new_cpu_data, 0, 0);
  }

  bool createInGPU(uint32_t new_num_bytes, int new_assigned_slot, const char* new_name) {
    assert(cte == nullptr);
    name = new_name;
    num_bytes = new_num_bytes;

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = num_bytes;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    HRESULT hr = Render.device->CreateBuffer(&bd, nullptr, &cte);

    assert(!FAILED(hr));
    setDXName(cte, name.c_str());

    assert(new_assigned_slot >= 0);
    slot = new_assigned_slot;
    return true;
  }
  
  void destroy() {
    SAFE_RELEASE(cte);
  }

  void activate() {
    assert(cte);
    PROFILE_FUNCTION(name);
    Render.ctx->VSSetConstantBuffers(slot, 1, &cte);
    Render.ctx->PSSetConstantBuffers(slot, 1, &cte);
  }

  void activateCS() {
    assert(cte);
    PROFILE_FUNCTION(name);
    Render.ctx->CSSetConstantBuffers(slot, 1, &cte);
  }

  void activateInCS(int cs_slot) {
    Render.ctx->CSSetConstantBuffers(cs_slot, 1, &cte);
  }

  const char* getName() const { return name.c_str(); }
  size_t size() const { return num_bytes; }

  virtual bool debugInMenu() { return false; }
};

// Default empty implementation to show a cte cpu in imgui
template< typename T>
bool debugCteInMenu(T&) { return false; }

// A combination of Data + BaseShaderCte
template< typename Data>
struct CShaderCte : public Data, public CBaseShaderCte {

public:

  bool create(int new_assigned_slot, const char* name) {
    bool is_ok = CBaseShaderCte::createInGPU(sizeof(Data), new_assigned_slot, name);
    return is_ok;
  }

  void updateFromCPU() {
    CBaseShaderCte::updateFromCPU( (Data*)this );
  }

  bool debugInMenu() override
  {
    if (ImGui::TreeNode(getName()))
    {
      bool changed = ::debugCteInMenu<Data>(*(Data*)this);
      ImGui::Text("Slot:%d Size:%d", slot, size());
      if (changed)
        updateFromCPU();
      ImGui::TreePop();
      return changed;
    }
    return false;
  }

};



