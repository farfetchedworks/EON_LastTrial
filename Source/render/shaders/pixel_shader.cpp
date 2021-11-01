#include "mcv_platform.h"
#include "compiler.h"

bool CPixelShader::create(const char* filename, const char* entry_point) {
  PROFILE_FUNCTION("PS");

  TBuffer blob;
  bool is_ok = compileShader(filename, entry_point, "ps_5_0", blob);
  assert(is_ok);

  // Create the vertex shader
  HRESULT hr = Render.device->CreatePixelShader(blob.data(), blob.size(), nullptr, &ps);
  assert(!FAILED(hr));

  return true;
}

void CPixelShader::destroy() {
  SAFE_RELEASE(ps);
}

void CPixelShader::activate() const {
  // Sometimes might be interesting to use a null ps (in the future)
  Render.ctx->PSSetShader(ps, nullptr, 0);
}

