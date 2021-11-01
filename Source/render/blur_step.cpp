#include "mcv_platform.h"
#include "blur_step.h"
#include "textures/render_to_texture.h"

extern CShaderCte<CtesBlur> cte_blur;

// ---------------------
bool CBlurStep::create(const char* name, int in_xres, int in_yres, DXGI_FORMAT fmt) {

  // Get input resolution
  xres = in_xres;
  yres = in_yres;

  rt_half_y = new CRenderToTexture();
  std::string sname = std::string(name) + "_y";
  bool is_ok = rt_half_y->createRT(sname.c_str(), xres, yres / 2, fmt, DXGI_FORMAT_UNKNOWN);
  assert(is_ok);

  sname = std::string(name) + "_xy";
  rt_output = new CRenderToTexture();
  is_ok = rt_output->createRT(sname.c_str(), xres / 2, yres / 2, fmt, DXGI_FORMAT_UNKNOWN);
  assert(is_ok);

  pipeline = Resources.get("blur.pipeline")->as<CPipelineState>();
  mesh = Resources.get("unit_quad_xy.mesh")->as<CMesh>();

  return is_ok;
}

void CBlurStep::applyBlur(float dx, float dy)
{
  cte_blur.blur_step.x = dx / (float)xres;
  cte_blur.blur_step.y = dy / (float)yres;
  cte_blur.activate();
  cte_blur.updateFromCPU();
  mesh->render();
}

CTexture* CBlurStep::apply(CTexture* input, float global_distance, VEC4 distances, VEC4 weights)
{

  mesh->activate();
  pipeline->activate();

  // Sum( Weights ) = 1 to not loose energy. +2 is to account for left and right taps
  float normalization_factor =
      1 * weights.x
    + 2 * weights.y
    + 2 * weights.z
    + 2 * weights.w
    ;

  cte_blur.blur_w.x = weights.x / normalization_factor;
  cte_blur.blur_w.y = weights.y / normalization_factor;
  cte_blur.blur_w.z = weights.z / normalization_factor;
  cte_blur.blur_w.w = weights.w / normalization_factor;
  
  // First tap is always in the center
  cte_blur.blur_d.x = distances.x;   
  cte_blur.blur_d.y = distances.y;
  cte_blur.blur_d.z = distances.z;
  cte_blur.blur_d.w = distances.w;    // Not used

  // First, blur vertically...
  rt_half_y->activateRT();
  input->activate(TS_ALBEDO);
  applyBlur(0, global_distance);

  // Now, render in the rt which we will return
  rt_output->activateRT();
  rt_half_y->activate(TS_ALBEDO);
  applyBlur(global_distance, 0);

  return rt_output;
}
