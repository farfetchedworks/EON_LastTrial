#pragma once

#include "texture.h"

class CRenderToTexture : public CTexture {

  DXGI_FORMAT         color_format = DXGI_FORMAT_UNKNOWN;
  DXGI_FORMAT         depth_format = DXGI_FORMAT_UNKNOWN;

  ID3D11RenderTargetView** render_target_views = nullptr;
  ID3D11DepthStencilView* depth_stencil_view = nullptr;

  // To be able to use the ZBuffer as a texture from our material system
  ID3D11Texture2D* depth_resource = nullptr;
  CTexture*        ztexture = nullptr;

  int                      xres = 0;
  int                      yres = 0;
  float                    pool_id = 0.f;
  int                      array_size = 0;

  static CRenderToTexture* current_rt;

public:

  bool createRT(const char* name, int new_xres, int new_yres
    , DXGI_FORMAT new_color_format
    , DXGI_FORMAT new_depth_format = DXGI_FORMAT_UNKNOWN
    , bool        uses_depth_of_backbuffer = false
    , bool        uses_depth_of_backbuffer_irradiance = false
    , bool        is_cubemap = false
    , int         array_size = 1
    , bool        is_uav = false
  );
  void destroy();

  CRenderToTexture* activateRT(ID3D11DepthStencilView* new_depth_stencil_view = nullptr, int array_index = 0);
  static void deactivate(int num_render_targets = 1 );
  void activateViewport();

  void clear(VEC4 clear_color, int array_index = 0);
  void clearZ();

  ID3D11RenderTargetView* getRenderTargetView(int array_index = 0) {
    return render_target_views[array_index];
  }
  ID3D11DepthStencilView* getDepthStencilView() {
    return depth_stencil_view;
  }

  static CRenderToTexture* getCurrentRT() { return current_rt; }
  CTexture* getZTexture() { return ztexture; }
  int getWidth() const { return xres; }
  int getHeight() const { return yres; }
  float getPoolId() const { return pool_id; };
  void setPoolId(float id) { pool_id = id; };
  bool renderInMenu() const override;
};
