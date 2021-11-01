#include "mcv_platform.h"
#include "render.h"
#include "render_utils.h"

// ---------------------------------------------------------------
// Reads a DX11 format from a string
DXGI_FORMAT readFormat(const json& j, const std::string& label) {
  std::string format = j.value(label, "");
  assert(!format.empty());

  if (format == "none")
    return DXGI_FORMAT_UNKNOWN;
  if (format == "R8G8B8A8_UNORM")
    return DXGI_FORMAT_R8G8B8A8_UNORM;
  if (format == "R32_TYPELESS")
    return DXGI_FORMAT_R32_TYPELESS;
  if (format == "R16_TYPELESS")
    return DXGI_FORMAT_R16_TYPELESS;
  if (format == "R8_UNORM")
    return DXGI_FORMAT_R8_UNORM;
  if (format == "R16_FLOAT")
    return DXGI_FORMAT_R16_FLOAT;
  if (format == "R16G16_FLOAT")
    return DXGI_FORMAT_R16G16_FLOAT;

  return DXGI_FORMAT_UNKNOWN;
}

// ---------------------------------------------------------------
int numChannelsInFormat(DXGI_FORMAT format) {

    switch (format) {
    case DXGI_FORMAT_UNKNOWN: return 0;
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_R32_FLOAT: return 1;
    case DXGI_FORMAT_R16G16_FLOAT: return 2;
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_FLOAT: return 4;
    };

    return 0;
}

// ---------------------------------------------------------------
int numBytesPerChannelInFormat(DXGI_FORMAT format) {

    switch (format) {
    case DXGI_FORMAT_UNKNOWN: return 0;
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8_UNORM: return 1;
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_FLOAT: return 2;
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_R32G32B32A32_FLOAT: return 4;
    };

    return 0;
}

// ---------------------------------------
struct CZConfigs {
  ID3D11DepthStencilState* z_cfgs[(int)ZConfig::COUNT];
  const char* names[(int)ZConfig::COUNT];

  bool add(const D3D11_DEPTH_STENCIL_DESC& desc, ZConfig cfg, const char* name) {
    // Create the dx obj in the slot 'cfg'
    HRESULT hr = Render.device->CreateDepthStencilState(&desc, &z_cfgs[(int)cfg]);
    if (FAILED(hr))
      return false;
    // Assing the name
    setDXName(z_cfgs[(int)cfg], name);
    // Save also the name for the ui
    names[(int)cfg] = name;
    return true;
  }

  bool create() {
    D3D11_DEPTH_STENCIL_DESC desc;
    memset(&desc, 0x00, sizeof(desc));
    desc.DepthEnable = FALSE;
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
    desc.StencilEnable = FALSE;
    if (!add(desc, ZConfig::DISABLE_ALL, "disable_all"))
      return false;

    // Default app, only pass those which are near than the previous samples
    memset(&desc, 0x00, sizeof(desc));
    desc.DepthEnable = TRUE;
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    desc.DepthFunc = D3D11_COMPARISON_LESS;
    desc.StencilEnable = FALSE;
    if (!add(desc, ZConfig::DEFAULT, "default"))
      return false;

    // test but don't write. Used while rendering particles for example
    memset(&desc, 0x00, sizeof(desc));
    desc.DepthEnable = true;
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;    // don't write
    desc.DepthFunc = D3D11_COMPARISON_LESS;               // only near z
    desc.StencilEnable = false;
    if (!add(desc, ZConfig::TEST_BUT_NO_WRITE, "test_but_no_write"))
      return false;

    // test for equal but don't write. Used to render on those pixels where
    // we have render previously like wireframes
    memset(&desc, 0x00, sizeof(desc));
    desc.DepthEnable = true;
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;    // don't write
    desc.DepthFunc = D3D11_COMPARISON_EQUAL;
    desc.StencilEnable = false;
    if (!add(desc, ZConfig::TEST_EQUAL, "test_equal"))
      return false;

    // Inverse Z Test, don't write. Used while rendering the lights
    memset(&desc, 0x00, sizeof(desc));
    desc.DepthEnable = true;
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    desc.DepthFunc = D3D11_COMPARISON_GREATER;
    desc.StencilEnable = false;
    if (!add(desc, ZConfig::INVERSE_TEST_NO_WRITE, "inverse_test_no_write"))
      return false;

    // Same but with stencil READ
    desc.StencilEnable = true;
    desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
    desc.StencilReadMask = 0x01;

    // Stencil operations if pixel is front-facing
    desc.FrontFace.StencilFunc = D3D11_COMPARISON_NEVER;
    desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;

    // Stencil operations if pixel is back-facing
    desc.BackFace.StencilFunc = D3D11_COMPARISON_NOT_EQUAL;
    desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;

    if (!add(desc, ZConfig::INVERSE_TEST_NO_WRITE_READ_STENCIL, "inverse_test_no_write_stencil_read"))
        return false;

    // Default but writing stencil
    memset(&desc, 0x00, sizeof(desc));
    desc.DepthEnable = TRUE;
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    desc.DepthFunc = D3D11_COMPARISON_LESS;

    // Stencil test parameters
    desc.StencilEnable = true;
    desc.StencilWriteMask = 0x01;
    desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;

    // Stencil operations if pixel is front-facing
    desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;

    // Stencil operations if pixel is back-facing
    desc.BackFace.StencilFunc = D3D11_COMPARISON_NEVER;
    desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;

    if (!add(desc, ZConfig::WRITE_STENCIL, "stencil_write"))
      return false;

    // Draw only when stencil is not the reference value
    memset(&desc, 0x00, sizeof(desc));
    desc.DepthEnable = TRUE;
    desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    desc.DepthFunc = D3D11_COMPARISON_LESS;

    // Stencil test parameters
    desc.StencilEnable = true;
    desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
    desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;

    // Stencil operations if pixel is front-facing
    desc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
    desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;

    // Stencil operations if pixel is back-facing
    desc.BackFace.StencilFunc = D3D11_COMPARISON_NEVER;
    desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;

    if (!add(desc, ZConfig::READ_STENCIL, "stencil_read"))
      return false;

    return true;
  }

  void destroy() {
    for (int i = 0; i < (int)ZConfig::COUNT; ++i)
      SAFE_RELEASE(z_cfgs[i]);
  }

};


// -----------------------------------------------
struct CBlends {
  ID3D11BlendState* blend_states[(int)BlendConfig::COUNT];
  const char* names[(int)BlendConfig::COUNT];

  bool add(const D3D11_BLEND_DESC& desc, BlendConfig cfg, const char* name) {
    // Create the dx obj in the slot 'cfg'
    HRESULT hr = Render.device->CreateBlendState(&desc, &blend_states[(int)cfg]);
    if (FAILED(hr))
      return false;
    // Assing the name
    setDXName(blend_states[(int)cfg], name);
    // Save also the name for the ui
    names[(int)cfg] = name;
    return true;
  }

  bool create() {

    blend_states[(int)BlendConfig::DEFAULT] = nullptr;
    names[(int)BlendConfig::DEFAULT] = "default";

    D3D11_BLEND_DESC desc;

    // Combinative blending  SRC * src.a + DST * ( 1 - src.a )
    memset(&desc, 0x00, sizeof(desc));
    desc.RenderTarget[0].BlendEnable = TRUE;
    desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
    desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    if (!add(desc, BlendConfig::COMBINATIVE, "combinative"))
      return false;

    // Additive blending
    memset(&desc, 0x00, sizeof(desc));
    desc.RenderTarget[0].BlendEnable = TRUE;
    desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;      // Color must come premultiplied
    desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    if (!add(desc, BlendConfig::ADDITIVE, "additive"))
      return false;

    // Additive blending controlled by src alpha
    memset(&desc, 0x00, sizeof(desc));
    desc.RenderTarget[0].BlendEnable = TRUE;
    desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
    desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    if (!add(desc, BlendConfig::ADDITIVE_BY_SRC_ALPHA, "additive_by_src_alpha"))
      return false;

    // Combinative blending in the gbuffer
    memset(&desc, 0x00, sizeof(desc));
    desc.RenderTarget[0].BlendEnable = TRUE;
    desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
    desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    // Blend also the 4 render tarngets
    desc.RenderTarget[1] = desc.RenderTarget[0];      // Normal
    desc.RenderTarget[2] = desc.RenderTarget[0];      // Z
    desc.RenderTarget[3] = desc.RenderTarget[0];
    if (!add(desc, BlendConfig::COMBINATIVE_GBUFFER, "combinative_gbuffer"))
      return false;

    return true;
  }

  void destroy() {
    for (int i = 0; i < (int)BlendConfig::COUNT; ++i)
      SAFE_RELEASE(blend_states[i]);
  }
};




// ----------------------------------------------------
// We are not using this function here...
static ID3D11SamplerState* all_samplers[SAMPLERS_COUNT];

void activateSampler(int slot, eSamplerType cfg) {
  Render.ctx->VSSetSamplers(slot, 1, &all_samplers[(int)cfg]);
  Render.ctx->PSSetSamplers(slot, 1, &all_samplers[(int)cfg]);
}

void activateAllSamplers() {
  // Activate all sampler in one API call
  Render.ctx->VSSetSamplers(0, SAMPLERS_COUNT, all_samplers);
  Render.ctx->PSSetSamplers(0, SAMPLERS_COUNT, all_samplers);
}

bool createSamplers() {
  HRESULT hr;

  // SAMPLER_WRAP_LINEAR
  {
    D3D11_SAMPLER_DESC desc = {};
    //desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    desc.Filter = D3D11_FILTER_ANISOTROPIC;
    desc.MaxAnisotropy = 8;
    desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    desc.MinLOD = 0;
    desc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = Render.device->CreateSamplerState(&desc, &all_samplers[SAMPLER_WRAP_LINEAR]);
    assert(!FAILED(hr));
  }

  // SAMPLER_CLAMP_LINEAR
  {
    D3D11_SAMPLER_DESC desc = {};
    desc.Filter = D3D11_FILTER_ANISOTROPIC;
    desc.MaxAnisotropy = 8;
    desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    desc.MinLOD = 0;
    desc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = Render.device->CreateSamplerState(&desc, &all_samplers[SAMPLER_CLAMP_LINEAR]);
    assert(!FAILED(hr));
  }

  // SAMPLER_CLAMP_NEAREST
  {
      D3D11_SAMPLER_DESC desc = {};
      desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
      desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
      desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
      desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
      desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
      desc.MinLOD = 0;
      desc.MaxLOD = D3D11_FLOAT32_MAX;
      hr = Render.device->CreateSamplerState(&desc, &all_samplers[SAMPLER_CLAMP_NEAREST]);
      assert(!FAILED(hr));
  }

  // SAMPLER_BORDER_LINEAR
  { // When we sample with texture coords out of the range 0...1 assign a specific color
    D3D11_SAMPLER_DESC desc = {};
    desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    desc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
    desc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
    desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
    desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    desc.BorderColor[0] = 0;    // in the range 0..255
    desc.BorderColor[1] = 0;
    desc.BorderColor[2] = 0;
    desc.BorderColor[3] = 0;
    desc.MinLOD = 0;
    desc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = Render.device->CreateSamplerState(&desc, &all_samplers[SAMPLER_BORDER_LINEAR]);
    assert(!FAILED(hr));
  }

  // SAMPLER_BORDER_PCF
  { // When we sample with texture coords out of the range 0...1 assign a specific color
    D3D11_SAMPLER_DESC desc = {
      D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,// D3D11_FILTER Filter;
      D3D11_TEXTURE_ADDRESS_BORDER, //D3D11_TEXTURE_ADDRESS_MODE AddressU;
      D3D11_TEXTURE_ADDRESS_BORDER, //D3D11_TEXTURE_ADDRESS_MODE AddressV;
      D3D11_TEXTURE_ADDRESS_BORDER, //D3D11_TEXTURE_ADDRESS_MODE AddressW;
      0,//FLOAT MipLODBias;
      0,//UINT MaxAnisotropy;
      D3D11_COMPARISON_LESS, //D3D11_COMPARISON_FUNC ComparisonFunc;
      0.0, 0.0, 0.0, 0.0,//FLOAT BorderColor[ 4 ];
      0,//FLOAT MinLOD;
      0//FLOAT MaxLOD;
    };
    hr = Render.device->CreateSamplerState(&desc, &all_samplers[SAMPLER_BORDER_PCF]);
    assert(!FAILED(hr));
  }

  // Other sampler types...

  return true;
}

void destroySamplers() {
  for (int i = 0; i < SAMPLERS_COUNT; ++i)
    SAFE_RELEASE(all_samplers[i]);
}

// ----------------------------------------------------
struct CRasterizers {

  ID3D11RasterizerState* rasterize_states[(int)RSConfig::COUNT];
  const char* names[(int)RSConfig::COUNT];

  bool add(const D3D11_RASTERIZER_DESC& desc, RSConfig cfg, const char* name) {
    // Create the dx obj in the slot 'cfg'
    HRESULT hr = Render.device->CreateRasterizerState(&desc, &rasterize_states[(int)cfg]);
    if (FAILED(hr))
      return false;
    // Assing the name
    setDXName(rasterize_states[(int)cfg], name);
    // Save also the name for the ui
    names[(int)cfg] = name;
    return true;
  }

  bool create() {

    rasterize_states[(int)RSConfig::DEFAULT] = nullptr;
    names[(int)RSConfig::DEFAULT] = "default";

    // Depth bias options when rendering the shadows
    D3D11_RASTERIZER_DESC desc = {
      D3D11_FILL_WIREFRAME, // D3D11_FILL_MODE FillMode;
      D3D11_CULL_NONE,  // D3D11_CULL_MODE CullMode;
      FALSE,            // BOOL FrontCounterClockwise;
      13,               // INT DepthBias;
      0.0f,             // FLOAT DepthBiasClamp;
      4.0f,             // FLOAT SlopeScaledDepthBias;
      TRUE,             // BOOL DepthClipEnable;
      FALSE,            // BOOL ScissorEnable;
      FALSE,            // BOOL MultisampleEnable;
      FALSE,            // BOOL AntialiasedLineEnable;
    };

    // Wireframe and default culling back
    if (!add(desc, RSConfig::WIREFRAME, "wireframe"))
      return false;

    // No culling at all
    desc = {
      D3D11_FILL_SOLID, // D3D11_FILL_MODE FillMode;
      D3D11_CULL_NONE,  // D3D11_CULL_MODE CullMode;
      FALSE,            // BOOL FrontCounterClockwise;
      0,                // INT DepthBias;
      0.0f,             // FLOAT DepthBiasClamp;
      0.0,              // FLOAT SlopeScaledDepthBias;
      TRUE,             // BOOL DepthClipEnable;
      FALSE,            // BOOL ScissorEnable;
      FALSE,            // BOOL MultisampleEnable;
      FALSE,            // BOOL AntialiasedLineEnable;
    };
    if (!add(desc, RSConfig::CULL_NONE, "cull_none"))
        return false;

    // Culling is reversed. Used when rendering the light volumes
    desc = {
      D3D11_FILL_SOLID, // D3D11_FILL_MODE FillMode;
      D3D11_CULL_FRONT, // D3D11_CULL_MODE CullMode;
      FALSE,            // BOOL FrontCounterClockwise;
      0,                // INT DepthBias;
      0.0f,             // FLOAT DepthBiasClamp;
      0.0,              // FLOAT SlopeScaledDepthBias;
      TRUE,             // BOOL DepthClipEnable;
      FALSE,            // BOOL ScissorEnable;
      FALSE,            // BOOL MultisampleEnable;
      FALSE,            // BOOL AntialiasedLineEnable;
    };

    if (!add(desc, RSConfig::REVERSE_CULLING, "reverse_culling"))
      return false;

    // Depth bias options when rendering the shadows ( for the DXSDk sample)
    desc = {
      D3D11_FILL_SOLID, // D3D11_FILL_MODE FillMode;
      D3D11_CULL_BACK,  // D3D11_CULL_MODE CullMode;
      FALSE,            // BOOL FrontCounterClockwise;
      13,               // INT DepthBias;
      0.0f,             // FLOAT DepthBiasClamp;
      4.0f,             // FLOAT SlopeScaledDepthBias;
      TRUE,             // BOOL DepthClipEnable;
      FALSE,            // BOOL ScissorEnable;
      FALSE,            // BOOL MultisampleEnable;
      FALSE,            // BOOL AntialiasedLineEnable;
    };
    
    if (!add(desc, RSConfig::SHADOWS, "shadows"))
      return false;

    return true;
  }

  void destroy() {
    for (int i = 0; i < (int)RSConfig::COUNT; ++i)
      SAFE_RELEASE(rasterize_states[i]);
  }

};

// ----------------------------------------------------
static CRasterizers rasterizers;
static CZConfigs    zconfigs;
static CBlends      blends;

// ----------------------------------------------------
// Public functions exposed to the rest of the engine
void activateRSConfig(enum RSConfig cfg) {
  Render.ctx->RSSetState(rasterizers.rasterize_states[(int)cfg]);
}

void activateZConfig(enum ZConfig cfg, uint32_t stencil_ref) {
  assert(zconfigs.z_cfgs[(int)cfg] != nullptr);
  Render.ctx->OMSetDepthStencilState(zconfigs.z_cfgs[(int)cfg], stencil_ref);
}

void activateBlendConfig(enum BlendConfig cfg) {
  float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };    // Not used
  UINT sampleMask = 0xffffffff;
  Render.ctx->OMSetBlendState(blends.blend_states[(int)cfg], blendFactor, sampleMask);
}

// ---------------------------------------
RSConfig RSConfigFromString(const std::string& aname) {
  for (int i = 0; i < (int)RSConfig::COUNT; ++i) {
    if (rasterizers.names[i] == aname)
      return RSConfig(i);
  }
  fatal("Invalid rsconfig name %s\n", aname.c_str());
  return RSConfig::DEFAULT;
}

bool renderInMenu(RSConfig& cfg) {
  return ImGui::Combo("RSConfig", (int*)&cfg, rasterizers.names, (int)RSConfig::COUNT);
}

// ---------------------------------------
ZConfig ZConfigFromString(const std::string& aname) {
  for (int i = 0; i < (int)ZConfig::COUNT; ++i) {
    if (zconfigs.names[i] == aname)
      return ZConfig(i);
  }
  fatal("Invalid zconfig name %s\n", aname.c_str());
  return ZConfig::DEFAULT;
}

bool renderInMenu(ZConfig& cfg) {
  return ImGui::Combo("ZConfig", (int*)&cfg, zconfigs.names, (int)ZConfig::COUNT);
}

// ---------------------------------------
BlendConfig BlendConfigFromString(const std::string& aname) {
  for (int i = 0; i < (int)BlendConfig::COUNT; ++i) {
    if (blends.names[i] == aname)
      return BlendConfig(i);
  }
  fatal("Invalid blend_config name %s\n", aname.c_str());
  return BlendConfig::DEFAULT;
}

bool renderInMenu(BlendConfig& cfg) {
  return ImGui::Combo("BlendConfig", (int*)&cfg, blends.names, (int)BlendConfig::COUNT);
}

// ---------------------------------------
bool createRenderUtils() {
  bool is_ok = true;
  is_ok &= createSamplers();
  is_ok &= zconfigs.create();
  is_ok &= rasterizers.create();
  is_ok &= blends.create();
  // ...
  return is_ok;
}

void destroyRenderUtils() {
  blends.destroy();
  rasterizers.destroy();
  zconfigs.destroy();
  destroySamplers();
}

void activateDefaultRenderState() {
  activateAllSamplers();
  activateZConfig(ZConfig::DEFAULT);
  activateRSConfig(RSConfig::DEFAULT);
  activateBlendConfig(BlendConfig::DEFAULT);
}

