#include "mcv_platform.h"
#include "compiler.h"
#include "render/vertex_declaration.h"

bool CVertexShader::create(const char* filename, const char* entry_point, const char* new_vertex_decl) {
  PROFILE_FUNCTION("VS");

  TBuffer buffer;
  bool is_ok = compileShader(filename, entry_point, "vs_5_0", buffer);
  assert(is_ok);

  // Create the vertex shader
  HRESULT hr;
  {
    PROFILE_FUNCTION("Create");
    hr = Render.device->CreateVertexShader(buffer.data(), buffer.size(), NULL, &vs);
    assert(!FAILED(hr));
  }

  {
    PROFILE_FUNCTION("VtxDecl");
    vertex_decl = getVertexDeclarationByName(new_vertex_decl);
    assert(vertex_decl);
  }

  // Create the input layout 
  {
    PROFILE_FUNCTION("Layout");
    hr = Render.device->CreateInputLayout(vertex_decl->layout, vertex_decl->num_elements, buffer.data(),
      buffer.size(), &vertex_layout);
    assert(!FAILED(hr));
  }

  setDXName(vertex_layout, new_vertex_decl);

  if (FAILED(hr))
    return false;

  return true;
}

void CVertexShader::destroy() {
  SAFE_RELEASE(vertex_layout);
  SAFE_RELEASE(vs);
}

void CVertexShader::activate() const {
  assert(vs);
  // Set the input layout
  Render.ctx->IASetInputLayout(vertex_layout);
  Render.ctx->VSSetShader(vs, NULL, 0);
}

void CVertexShader::deactivateResources()
{
  // Can't update a buffer if it's still bound in a vs. So unbound it.
  ID3D11ShaderResourceView* srv_nulls[3] = { nullptr, nullptr, nullptr };
  Render.ctx->VSSetShaderResources(0, 3, srv_nulls);
}
