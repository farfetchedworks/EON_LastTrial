#pragma once

#include "resources/resources.h"
#include <d3d11shader.h>

class CComputeShader : public IResource
{
  ID3D11ComputeShader* cs = nullptr;
  json                 jdef;
  void scanParameters(TBuffer& buffer);

public:

  std::vector<D3D11_SHADER_INPUT_BIND_DESC> bound_resources;
  uint32_t                                  thread_group_size[3];

  bool create(const json& j);
  void destroy();
  void activate() const;
  void onFileChange(const std::string& filename) override;
  bool renderInMenu() const override;
  static void deactivate();

};
