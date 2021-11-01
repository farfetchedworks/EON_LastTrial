#include "mcv_platform.h"
#include "compute_shader.h"
#include "render/shaders/compiler.h"

#include <d3dcompiler.inl>    // D3D11Reflect
FORCEINLINE HRESULT
D3D11Reflect(_In_reads_bytes_(SrcDataSize) LPCVOID pSrcData,
  _In_ SIZE_T SrcDataSize,
  _Out_ ID3D11ShaderReflection** ppReflector)
{
  return D3DReflect(pSrcData, SrcDataSize,
    IID_ID3D11ShaderReflection, (void**)ppReflector);
}

class CComputeResourceType : public CResourceType {
public:
  const char* getExtension(int idx) const { return ".compute"; }
  const char* getName() const { return "Compute Shader"; }
  IResource* create(const std::string& name) const {
    fatal("ComputeShaders from source not implemented. Register the shader %s in data/computes.json\n", name.c_str());
    return nullptr;
  }
};

// -----------------------------------------
template<>
CResourceType* getClassResourceType<CComputeShader>() {
  static CComputeResourceType factory;
  return &factory;
}

void CComputeShader::onFileChange(const std::string& filename) {
  if (filename == jdef["cs_fx"].get< std::string >())
    create(jdef);
}

// -----------------------------------------
bool CComputeShader::create(const json& j)
{
  jdef = j;
  std::string fx = j.value("cs_fx", "");
  assert(!fx.empty());
  std::string fn = j.value("cs_fn", "");
  assert(!fn.empty());
  std::string profile = j.value("profile", "cs_5_0");

  TBuffer buf;
  HRESULT hr = compileShader(fx.c_str(), fn.c_str(), profile.c_str(), buf);
  if (FAILED(hr))
    return false;

  // Create the compute shader
  hr = Render.device->CreateComputeShader(
      buf.data()
    , buf.size()
    , nullptr
    , &cs);
  if (FAILED(hr))
    return false;

  setDXName(cs, (fx + ":" + fn).c_str());

  scanParameters(buf);

  return true;
}

bool CComputeShader::renderInMenu() const
{
  ImGui::Text("Thread Group Sizes: %d x %d x %d", thread_group_size[0], thread_group_size[1], thread_group_size[2]);

  static const char* type_names[] = {
      "CBUFFER",
      "TBUFFER",
      "TEXTURE",
      "SAMPLER",
      "UAV_RWTYPED",
      "STRUCTURED",
      "UAV_RWSTRUCTURED",
      "BYTEADDRESS",
      "UAV_RWBYTEADDRESS",
      "UAV_APPEND_STRUCTURED",
      "UAV_CONSUME_STRUCTURED",
      "UAV_RWSTRUCTURED_WITH_COUNTER",
  };

  for (auto it : bound_resources)
  {
    if (ImGui::TreeNode(it.Name))
    {
      const char* str = "unknown type";
      if (it.Type <= D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER)
        str = type_names[it.Type];
      ImGui::LabelText("Type", "%s (%d)", str, it.Type);
      ImGui::LabelText("Bind Point", "%d (x%d)", it.BindPoint, it.BindCount);
      ImGui::LabelText("NumSamples", "%d", it.NumSamples);
      ImGui::LabelText("Flags", "%08x", it.uFlags);
      ImGui::TreePop();
    }
  }
  return false;
}

void CComputeShader::scanParameters(TBuffer& buffer)
{
  // Query bound resources
  ID3D11ShaderReflection* reflection = NULL;

  D3D11Reflect(buffer.data(), buffer.size(), &reflection);
  D3D11_SHADER_DESC desc = {};
  reflection->GetDesc(&desc);
  
  // Use the reflection api to scan the description of each bounded resource associated to the shader
  bound_resources.clear();
  for (unsigned int i = 0; i < desc.BoundResources; ++i)
  {
    D3D11_SHADER_INPUT_BIND_DESC desc;
    HRESULT hr = reflection->GetResourceBindingDesc(i, &desc);
    if (FAILED(hr))
      continue;
    // Do a copy of the name or it will be lost
    desc.Name = _strdup(desc.Name);
    bound_resources.push_back(desc);
  }

  // Scan the group sizes defined in the cs source code (5,1,1)
  reflection->GetThreadGroupSize(&thread_group_size[0], &thread_group_size[1], &thread_group_size[2]);
  reflection->Release();
  
}

void CComputeShader::destroy()
{
  SAFE_RELEASE(cs);
}

void CComputeShader::activate() const
{
  Render.ctx->CSSetShader(cs, nullptr, 0);
}

void CComputeShader::deactivate() {
  static const int max_slots = 4;
  ID3D11UnorderedAccessView* uavs_null[max_slots] = { nullptr, nullptr, nullptr, nullptr };
  ID3D11ShaderResourceView* srvs_null[max_slots] = { nullptr, nullptr, nullptr, nullptr };
  // Null all cs params
  Render.ctx->CSSetUnorderedAccessViews(0, max_slots, uavs_null, nullptr);
  Render.ctx->CSSetShaderResources(0, max_slots, srvs_null);
  Render.ctx->CSSetShader(nullptr, nullptr, 0);
};