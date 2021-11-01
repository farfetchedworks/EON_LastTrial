

#include "mcv_platform.h"
#include <d3dcompiler.h>
#include "utils/cache_manager.h"

#pragma comment(lib, "D3DCompiler.lib")

CCacheManager cached_shaders("Cache Shaders", "data/cache/");

//--------------------------------------------------------------------------------------
// Helper for compiling shaders with D3DX11
//--------------------------------------------------------------------------------------
bool compileShader(const char* filename, const char* entry_point, const char* shader_model, TBuffer& out_buffer) {
  PROFILE_FUNCTION("compile");

  char cache_name[256];
  snprintf(cache_name, sizeof(cache_name) - 1, "%s_Fn_%s_SM_%s", filename, entry_point, shader_model);
  if (cached_shaders.isCached(filename, cache_name, out_buffer))
    return true;

  HRESULT hr = S_OK;

  DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
  // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
  // Setting this flag improves the shader debugging experience, but still allows 
  // the shaders to be optimized and to run exactly the way they will run in 
  // the release configuration of this program.
  dwShaderFlags |= D3DCOMPILE_DEBUG;
  dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

  // To avoid having to use Transpose on every matrix we upload to the GPU
  dwShaderFlags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;

  D3D_SHADER_MACRO shaderMacros[] = {
#ifdef NDEBUG
      { "RELEASE_VERSION",             "1" },
#endif
      { NULL, NULL }
  };

  // Convert utf8 to wide char
  wchar_t wFilename[MAX_PATH];
  mbstowcs(wFilename, filename, MAX_PATH);

  // Try forever....
  ID3DBlob* pBlobOut = nullptr;
  while (true) {
    ID3DBlob* pErrorBlob = nullptr;
    
    // Try to compile...
    hr = D3DCompileFromFile(
      wFilename, 
      shaderMacros,           // Macros
      D3D_COMPILE_STANDARD_FILE_INCLUDE,  // To find includes in the current .hlsl source code
      entry_point,
      shader_model,
      dwShaderFlags, 0,
      &pBlobOut,
      &pErrorBlob);

    if (FAILED(hr)) {
      // Do we have an error msg?
      if (pErrorBlob) {
        fatal("Error compiling shader %s %s@%s\n%s",
          shader_model,
          entry_point,
          filename,
          (char*)pErrorBlob->GetBufferPointer()
        );
        pErrorBlob->Release();
      }
      else if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
        fatal("Can't find shader %s\nInput file not found", filename);
      }
      else {
        fatal("Can't compile shader %s. Unknown error code %08x\n", filename, hr);
      }
      continue;
    }
    if (pErrorBlob) pErrorBlob->Release();

    // At this point, the compilation is OK, so we can exit the forever loop
    break;
  }

  // Ok, file has been compiled by HLSL compiler
  out_buffer.resize(pBlobOut->GetBufferSize());
  memcpy(out_buffer.data(), pBlobOut->GetBufferPointer(), pBlobOut->GetBufferSize());

  cached_shaders.cache(filename, cache_name, out_buffer);

  return true;
}
