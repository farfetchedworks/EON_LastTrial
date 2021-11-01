#include "mcv_platform.h"
#include "render_to_texture.h"

CRenderToTexture* CRenderToTexture::current_rt = nullptr;

// ---------------------------------------------
bool createDepthStencil(
  const std::string& aname,
  int width, int height,
  DXGI_FORMAT format,
  // outputs
  ID3D11Texture2D** depth_stencil_resource,
  ID3D11DepthStencilView** depth_stencil_view,
  CTexture** out_ztexture
) {

  assert(format == DXGI_FORMAT_R32_TYPELESS
    || format == DXGI_FORMAT_R24G8_TYPELESS
    || format == DXGI_FORMAT_R16_TYPELESS
    || format == DXGI_FORMAT_D24_UNORM_S8_UINT
    || format == DXGI_FORMAT_R8_TYPELESS);

  // Crear un ZBuffer de la resolucion de mi backbuffer
  D3D11_TEXTURE2D_DESC desc;
  ZeroMemory(&desc, sizeof(desc));
  desc.Width = width;
  desc.Height = height;
  desc.MipLevels = 1;
  desc.ArraySize = 1;
  desc.Format = format;
  desc.SampleDesc.Count = 1;
  desc.SampleDesc.Quality = 0;
  desc.Usage = D3D11_USAGE_DEFAULT;
  desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
  desc.CPUAccessFlags = 0;
  desc.MiscFlags = 0;

  // The format 'DXGI_FORMAT_D24_UNORM_S8_UINT' can't be binded to shader resource
  if (format != DXGI_FORMAT_D24_UNORM_S8_UINT)
    desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;

  // SRV = Shader Resource View
  // DSV = Depth Stencil View
  DXGI_FORMAT texturefmt = DXGI_FORMAT_R32_TYPELESS;
  DXGI_FORMAT SRVfmt = DXGI_FORMAT_R32_FLOAT;       // Stencil format
  DXGI_FORMAT DSVfmt = DXGI_FORMAT_D32_FLOAT;       // Depth format

  switch (format) {
  case DXGI_FORMAT_R32_TYPELESS:
    SRVfmt = DXGI_FORMAT_R32_FLOAT;
    DSVfmt = DXGI_FORMAT_D32_FLOAT;
    break;
  case DXGI_FORMAT_R24G8_TYPELESS:
    SRVfmt = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    DSVfmt = DXGI_FORMAT_D24_UNORM_S8_UINT;
    break;
  case DXGI_FORMAT_R16_TYPELESS:
    SRVfmt = DXGI_FORMAT_R16_UNORM;
    DSVfmt = DXGI_FORMAT_D16_UNORM;
    break;
  case DXGI_FORMAT_R8_TYPELESS:
    SRVfmt = DXGI_FORMAT_R8_UNORM;
    DSVfmt = DXGI_FORMAT_R8_UNORM;
    break;
  case DXGI_FORMAT_D24_UNORM_S8_UINT:
    SRVfmt = desc.Format;
    DSVfmt = desc.Format;
    break;
  default:
    fatal("Unsupported format creating depth buffer\n");
  }

  HRESULT hr = Render.device->CreateTexture2D(&desc, NULL, depth_stencil_resource);
  if (FAILED(hr))
    return false;
  setDXName(*depth_stencil_resource, aname.c_str());

  // Create the depth stencil view
  D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
  ZeroMemory(&descDSV, sizeof(descDSV));
  descDSV.Format = DSVfmt;
  descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
  descDSV.Texture2D.MipSlice = 0;
  hr = Render.device->CreateDepthStencilView(*depth_stencil_resource, &descDSV, depth_stencil_view);
  if (FAILED(hr))
    return false;
  setDXName(*depth_stencil_view, (aname + "_DSV").c_str());

  if (out_ztexture) {
    // Setup the description of the shader resource view.
    D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
    shaderResourceViewDesc.Format = SRVfmt;
    shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
    shaderResourceViewDesc.Texture2D.MipLevels = desc.MipLevels;

    // Create the shader resource view.
    ID3D11ShaderResourceView* depth_resource_view = nullptr;
    hr = Render.device->CreateShaderResourceView(*depth_stencil_resource, &shaderResourceViewDesc, &depth_resource_view);
    if (FAILED(hr))
      return false;

    CTexture* ztexture = new CTexture();
    ztexture->setDXParams(*depth_stencil_resource, depth_resource_view);
    std::string zname = "Z" + aname;
    Resources.registerResource(ztexture, zname, getClassResourceType<CTexture>());
    setDXName(*depth_stencil_resource, (ztexture->getName() + "_DSR").c_str());
    setDXName(depth_resource_view, (ztexture->getName() + "_DRV").c_str());

    // The ztexture already got the reference
    depth_resource_view->Release();
    *out_ztexture = ztexture;
  }

  return true;
}

bool CRenderToTexture::createRT(const char* name, int new_xres, int new_yres
  , DXGI_FORMAT new_color_format
  , DXGI_FORMAT new_depth_format
  , bool        uses_depth_of_backbuffer
  , bool        uses_depth_of_backbuffer_irradiance
  , bool        is_array
  , int         new_array_size
  , bool        is_uav
) {

  destroy();

  xres = new_xres;
  yres = new_yres;

  this->array_size = new_array_size;
  render_target_views = new ID3D11RenderTargetView * [new_array_size];

  // Create color buffer
  color_format = new_color_format;
  if (color_format != DXGI_FORMAT_UNKNOWN) {

    if (!create(name, new_xres, new_yres, color_format, is_uav ? CREATE_RENDER_TARGET_UAV : CREATE_RENDER_TARGET, is_array, new_array_size))
      return false;

    //setDXName(texture, name);
    setDXName(shader_resource_view, name);

    // Rtv for cubemaps
    D3D11_RENDER_TARGET_VIEW_DESC rtv;
    rtv.Format = color_format;

    if (is_array) {
        rtv.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
        rtv.Texture2DArray.ArraySize = 1;
        rtv.Texture2DArray.MipSlice = 0;
    }
    else if (is_uav) {
        rtv.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        rtv.Texture2D.MipSlice = 0;
    }

    for (int i = 0; i < new_array_size; ++i) {

        //render_target_views[i] = new ID3D11RenderTargetView*();

        rtv.Texture2DArray.FirstArraySlice = i;
        // The part of the render target view
        HRESULT hr = Render.device->CreateRenderTargetView((ID3D11Resource*)texture, is_array || is_uav ? &rtv : nullptr, &(render_target_views[i]));
        if (FAILED(hr))
            return false;

        std::string rtv_name_str = (std::string(name) + std::to_string(i));
        const char* rtv_name = rtv_name_str.c_str();
        setDXName(render_target_views[i], rtv_name);
    }
  }

  // Resource information
  if (getName() != name) {
    Resources.registerResource(this, name, getClassResourceType<CTexture>());
  }

  // Create ZBuffer 
  depth_format = new_depth_format;
  if (depth_format != DXGI_FORMAT_UNKNOWN) {
    if (!createDepthStencil(getName(), xres, yres, new_depth_format, &depth_resource, &depth_stencil_view, &ztexture))
      return false;
    setDXName(depth_stencil_view, getName().c_str());
  } 
  else {
    // Create can have the option to use the ZBuffer of the backbuffer
    if (uses_depth_of_backbuffer_irradiance) {
      assert(new_xres == IR_SIZE);
      assert(new_yres == IR_SIZE);
      depth_stencil_view = Render.depth_stencil_view_irradiance;
      depth_stencil_view->AddRef();
    }
    else if (uses_depth_of_backbuffer) {
      assert(new_xres == Render.getWidth());
      assert(new_yres == Render.getHeight());
      depth_stencil_view = Render.depth_stencil_view;
      depth_stencil_view->AddRef();
    }
  }

  return true;
}

void CRenderToTexture::destroy() {
  if (color_format != DXGI_FORMAT_UNKNOWN) {
    for (int i = 0; i < array_size; ++i) {
      if (render_target_views[i] != nullptr) {
          SAFE_RELEASE(render_target_views[i]);
      }
    }
  }

  SAFE_RELEASE(depth_stencil_view);
  SAFE_RELEASE(depth_resource);
  CTexture::destroy();
}

CRenderToTexture* CRenderToTexture::activateRT(ID3D11DepthStencilView* new_depth_stencil_view, int array_index) {
  CRenderToTexture* prev_rt = current_rt;
  ID3D11RenderTargetView* null_rt = nullptr;
  Render.ctx->OMSetRenderTargets(1, 
      color_format != DXGI_FORMAT_UNKNOWN ? &(render_target_views[array_index]) : &null_rt, 
      new_depth_stencil_view ? new_depth_stencil_view : depth_stencil_view);
  activateViewport();
  current_rt = this;
  return prev_rt;
}

void CRenderToTexture::deactivate(int num_rts) {
  assert(num_rts <= 4);
  // Disable rendering to all render targets.
  ID3D11RenderTargetView* rt_nulls[4] = { nullptr, nullptr, nullptr, nullptr };
  Render.ctx->OMSetRenderTargets(num_rts, rt_nulls, nullptr);
}


void CRenderToTexture::activateViewport() {
  D3D11_VIEWPORT vp;
  vp.Width = (float)xres;
  vp.Height = (float)yres;
  vp.TopLeftX = 0;
  vp.TopLeftY = 0;
  vp.MinDepth = 0.f;
  vp.MaxDepth = 1.f;
  Render.ctx->RSSetViewports(1, &vp);
}

void CRenderToTexture::clear(VEC4 clear_color, int array_index) {
  assert(render_target_views[array_index]);
  Render.ctx->ClearRenderTargetView(render_target_views[array_index], &clear_color.x);
}

void CRenderToTexture::clearZ() {
  if (depth_stencil_view)
    Render.ctx->ClearDepthStencilView(depth_stencil_view, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

bool CRenderToTexture::renderInMenu() const {

  // If we have a color buffer...
  if (render_target_views[0]) {
      CTexture::renderInMenu();
  }

  // Show the Depth Buffer if exists
  if (depth_stencil_view && ztexture)
    ztexture->renderInMenu();

  return false;
}
