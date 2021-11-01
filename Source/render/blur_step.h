#pragma once

class CRenderToTexture;
class CPipelineState;
class CMesh;
class CTexture;

class CBlurStep {

  CRenderToTexture* rt_half_y = nullptr;
  CRenderToTexture* rt_output = nullptr;    // half x & y

  int   xres = 0, yres = 0;
  const CPipelineState* pipeline = nullptr;
  const CMesh* mesh = nullptr;

public:

  bool create( const char* name, int in_xres, int in_yres, DXGI_FORMAT fmt);
  void applyBlur(float dx, float dy);
  CTexture* apply(CTexture* input, float global_distance, VEC4 distances, VEC4 weights);
  CRenderToTexture* getRenderTarget() { return rt_output; }
};
