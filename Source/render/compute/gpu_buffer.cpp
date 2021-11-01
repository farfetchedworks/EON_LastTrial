#include "mcv_platform.h"
#include "gpu_buffer.h"
#include "../bin/data/shaders/constants_particles.h"

void CGPUBuffer::activate(int slot) const {
  Render.ctx->VSSetShaderResources(slot, 1, &srv);
}

void CGPUBuffer::activatePS(int slot) const {
  Render.ctx->PSSetShaderResources(slot, 1, &srv);
}

void CGPUBuffer::activateCSasSR(int slot) const
{
  Render.ctx->CSSetShaderResources(slot, 1, &srv);
}

void CGPUBuffer::activateCSasUAV(int slot) const
{
  UINT zero = 0;
  Render.ctx->CSSetUnorderedAccessViews(slot, 1, &uav, &zero);
}

bool CGPUBuffer::create(uint32_t new_bytes_per_elem, uint32_t new_num_elems, bool is_indirect, const std::string& new_name) {
  destroy();
  bool is_raw = false;

  bytes_per_elem = new_bytes_per_elem;
  num_elems = new_num_elems;
  name = new_name;

  uint32_t total_bytes = bytes_per_elem * num_elems;
  cpu_data.resize(total_bytes);

  assert(!buffer && !srv);

  // Set flags
  D3D11_BUFFER_DESC bd = {};
  bd.Usage = D3D11_USAGE_DEFAULT;
  bd.ByteWidth = (UINT)total_bytes;

  if (is_indirect) {
    bd.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    bd.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS | D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
    // All indirect buffers are raw
    is_raw = true;
  }
  else {
    bd.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
  }

  bd.StructureByteStride = bytes_per_elem;

  memset(cpu_data.data(), 0, num_elems * bytes_per_elem);

  D3D11_SUBRESOURCE_DATA data;
  data.pSysMem = (void*)cpu_data.data();
  data.SysMemPitch = 0;
  data.SysMemSlicePitch = 0;

  // Create the buffer in the GPU
  HRESULT hr = Render.device->CreateBuffer(&bd, &data, &buffer);
  if (FAILED(hr))
    return false;
  setDXName(buffer, name.c_str());

  // This is to be able to use it in a shader
  if (!is_indirect) {
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
    srv_desc.BufferEx.FirstElement = 0;
    srv_desc.Format = DXGI_FORMAT_UNKNOWN;
    srv_desc.BufferEx.NumElements = num_elems;
    if (FAILED(Render.device->CreateShaderResourceView(buffer, &srv_desc, &srv)))
      return false;
    std::string srv_str = (name + ".srv");
    setDXName(srv, srv_str.c_str());
  }

  // This is to be able to use it as a unordered access buffer
  D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc;
  ZeroMemory(&uav_desc, sizeof(uav_desc));
  uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
  uav_desc.Buffer.FirstElement = 0;
  // Format must be must be DXGI_FORMAT_UNKNOWN, when creating 
  // a View of a Structured Buffer
  uav_desc.Format = DXGI_FORMAT_UNKNOWN;
  uav_desc.Buffer.NumElements = num_elems;
  if (is_raw) {
    uav_desc.Format = DXGI_FORMAT_R32_TYPELESS;
    uav_desc.Buffer.Flags |= D3D11_BUFFER_UAV_FLAG_RAW;
    uav_desc.Buffer.NumElements = bd.ByteWidth / 4;
  }
  if (FAILED(Render.device->CreateUnorderedAccessView(buffer, &uav_desc, &uav)))
    return false;
  std::string uav_str = (name + ".uav");
  setDXName(uav, uav_str.c_str());

  return true;
}

bool CGPUBuffer::create(const json& j) {
  data_type_name.clear();
  bytes_per_elem = 0;

  if (j.count("data_type")) {
    data_type_name = j.value("data_type", "");
    if (data_type_name == "MAT44") {
      bytes_per_elem = sizeof(MAT44);
    }
    else if (data_type_name == "TParticle") {
      bytes_per_elem = sizeof(TParticle);
    }
    else {
      fatal("Don't know gpu buffer data type %s", data_type_name.c_str());
    }
  }

  bytes_per_elem = j.value("bytes_per_elem", bytes_per_elem);
  assert(bytes_per_elem > 0);
  num_elems = j.value("num_elems", 1);
  is_indirect = j.value("is_indirect", false);
  bool is_ok = create(bytes_per_elem, num_elems, is_indirect, j.value("name", ""));

  if (j.count("initial_data")) {
    if (data_type_name == "MAT44") {
      assert(bytes_per_elem == sizeof(MAT44));
      std::vector< MAT44 > transforms;
      for (auto it : j["initial_data"].items()) {
        const json& jinstance = it.value();
        CTransform transform;
        transform.fromJson(jinstance);
        transforms.push_back(transform.asMatrix());
      }
      memcpy(cpu_data.data(), transforms.data(), transforms.size() * sizeof(MAT44));
    }
    else {
      fatal("Don't know to read data type %s", data_type_name.c_str());
    }
    copyCPUtoGPU();
  }

  return is_ok;
}

void CGPUBuffer::destroy() {
  SAFE_RELEASE(buffer);
  SAFE_RELEASE(srv);
  SAFE_RELEASE(uav);
}

// This is a very slow operation!!!!
bool CGPUBuffer::copyGPUtoCPU() const {
  // We need a staging buffer to read the contents of the gpu buffer.
  // Create a new buffer with the same format as the buffer of the gpu buffer, but with the staging & cpu read flags.
  D3D11_BUFFER_DESC desc;
  ZeroMemory(&desc, sizeof(desc));
  buffer->GetDesc(&desc);
  UINT byteSize = desc.ByteWidth;
  desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  desc.Usage = D3D11_USAGE_STAGING;
  desc.BindFlags = 0;
  desc.MiscFlags = 0;
  ID3D11Buffer* cpu_buffer = nullptr;
  if (!FAILED(Render.device->CreateBuffer(&desc, nullptr, &cpu_buffer))) {
    // Copy from the gpu buffer to the cpu buffer.
    Render.ctx->CopyResource(cpu_buffer, buffer);

    assert(cpu_buffer);

    // Request access to read it.
    D3D11_MAPPED_SUBRESOURCE mr;
    if (Render.ctx->Map(cpu_buffer, 0, D3D11_MAP_READ, 0, &mr) == S_OK) {
      memcpy((char*)cpu_data.data(), mr.pData, desc.ByteWidth);
      Render.ctx->Unmap(cpu_buffer, 0);
      return true;
    }

    cpu_buffer->Release();
  }
  return false;
}

void CGPUBuffer::copyCPUtoGPU() const {
  assert(buffer);
  Render.ctx->UpdateSubresource(buffer, 0, nullptr, cpu_data.data(), 0, 0);
}
void CGPUBuffer::copyCPUtoGPUFrom(const void* new_data) const {
  assert(new_data);
  assert(buffer);
  // Ensure new_data points at least to cpu_data.size() bytes
  Render.ctx->UpdateSubresource(buffer, 0, nullptr, new_data, 0, 0);
}

bool CGPUBuffer::debugInMenu() {
  ImGui::Text("GPU Buffer %s (%ld bytes) %d x %d (%s)", name.c_str(), cpu_data.size(), num_elems, bytes_per_elem, data_type_name.c_str());
  
  if (!buffer)
    ImGui::Text("  GPU Buffer %s INVALID", name.c_str());

  if(is_indirect)
    ImGui::Text("  Indirect Buffer");

  if (data_type_name == "MAT44") {
    float rad = 5.0f;
    if (ImGui::SmallButton("Randomize...")) {
      std::vector< MAT44 > transforms;
      for (uint32_t i = 0; i < num_elems; ++i)
        transforms.push_back(MAT44::CreateTranslation(Random::range(-rad, rad), 0.0f, Random::range(-rad, rad)));
      copyCPUtoGPUFrom(transforms.data());
    }
  }
  return false;
}
