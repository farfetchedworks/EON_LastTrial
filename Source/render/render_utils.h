#pragma once

DXGI_FORMAT readFormat(const json& j, const std::string& label);
int numChannelsInFormat(DXGI_FORMAT format);
int numBytesPerChannelInFormat(DXGI_FORMAT format);

// ---------------------------------------
enum eSamplerType {
  // Order should match the sampler definition values in data/shaders/common.h
  SAMPLER_WRAP_LINEAR = 0,
  SAMPLER_BORDER_LINEAR,
  SAMPLER_BORDER_PCF,
  SAMPLER_CLAMP_LINEAR,
  SAMPLER_CLAMP_NEAREST,

  // ..
  SAMPLERS_COUNT
};
void activateSampler(int slot, eSamplerType cfg);

// ---------------------------------------
enum class ZConfig {
    DEFAULT
  , DISABLE_ALL
  , TEST_BUT_NO_WRITE
  , TEST_EQUAL
  , INVERSE_TEST_NO_WRITE
  , WRITE_STENCIL
  , READ_STENCIL
  , INVERSE_TEST_NO_WRITE_READ_STENCIL
  , COUNT
};
void activateZConfig(ZConfig cfg, uint32_t stencil_ref = 1);

// ---------------------------------------
enum class RSConfig {
  DEFAULT
  , REVERSE_CULLING
  , CULL_NONE
  , SHADOWS
  , WIREFRAME
  , COUNT
};
void activateRSConfig(RSConfig cfg);

// ---------------------------------------
enum class BlendConfig {
  DEFAULT
, ADDITIVE
, ADDITIVE_BY_SRC_ALPHA
, COMBINATIVE
, COMBINATIVE_GBUFFER
, COUNT
};
void activateBlendConfig(BlendConfig cfg);

// ---------------------------------------
// Read from name
ZConfig ZConfigFromString(const std::string& aname);
RSConfig RSConfigFromString(const std::string& aname);
BlendConfig BlendConfigFromString(const std::string& aname);

// ---------------------------------------
// Edit in ImGui
bool renderInMenu(ZConfig& cfg);
bool renderInMenu(RSConfig& cfg);
bool renderInMenu(BlendConfig& cfg);

// ---------------------------------------
bool createRenderUtils();
void destroyRenderUtils();
void activateDefaultRenderState();
