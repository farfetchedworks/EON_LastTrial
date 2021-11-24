#include "mcv_platform.h"
#include "shaders/shader_cte.h"
#include "render_utils.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3d9.lib")      // D3DPERF_*

CRender Render;

bool CRender::createDevice(HWND hWnd) {
  HRESULT hr = S_OK;

  RECT rc;
  GetClientRect(hWnd, &rc);
  UINT new_width = rc.right - rc.left;
  UINT new_height = rc.bottom - rc.top;

  UINT createDeviceFlags = 0;
#ifdef _DEBUG
  createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

  D3D_FEATURE_LEVEL featureLevels[] = {
      D3D_FEATURE_LEVEL_11_0,
  };
  UINT numFeatureLevels = ARRAYSIZE(featureLevels);

  DXGI_SWAP_CHAIN_DESC sd;
  ZeroMemory(&sd, sizeof(sd));
  sd.BufferCount = 1;
  sd.BufferDesc.Width = new_width;
  sd.BufferDesc.Height = new_height;
  sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  sd.BufferDesc.RefreshRate.Numerator = 60;
  sd.BufferDesc.RefreshRate.Denominator = 1;
  sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.OutputWindow = hWnd;
  sd.SampleDesc.Count = 1;
  sd.SampleDesc.Quality = 0;
  sd.Windowed = !fullscreen;

  D3D_FEATURE_LEVEL featureLevel;
  hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
    D3D11_SDK_VERSION, &sd, &swap_chain, &device, &featureLevel, &ctx);
  if (FAILED(hr))
    return false;

#ifdef _DEBUG
  hr = device->QueryInterface(__uuidof(ID3D11Debug), (void**)&debug);
  if (FAILED(hr))
      return false;
#endif

  // Create a render target view
  ID3D11Texture2D* pBackBuffer = NULL;
  hr = swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
  if (FAILED(hr))
    return false;

  hr = device->CreateRenderTargetView(pBackBuffer, NULL, &render_target_view);
  pBackBuffer->Release();
  if (FAILED(hr))
    return false;

  setDXName(render_target_view, "RenderTargetView");

  width = new_width;
  height = new_height;

  return true;
}

bool CRender::createDepthBuffer() {
  HRESULT hr;

  {
      D3D11_TEXTURE2D_DESC desc = {};
      desc.Width = (UINT)width;
      desc.Height = (UINT)height;
      desc.MipLevels = 1;
      desc.ArraySize = 1;
      desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
      desc.SampleDesc.Count = 1;
      desc.SampleDesc.Quality = 0;
      desc.Usage = D3D11_USAGE_DEFAULT;
      desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
      desc.CPUAccessFlags = 0;
      desc.MiscFlags = 0;
      hr = device->CreateTexture2D(&desc, nullptr, &depth_stencil);
      if (FAILED(hr))
          return false;

      // To use the previous texture as a zbuffer, I need a depth stencil VIEW
      D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
      descDSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
      descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
      descDSV.Texture2D.MipSlice = 0;
      hr = device->CreateDepthStencilView(depth_stencil, &descDSV, &depth_stencil_view);
      if (FAILED(hr))
          return false;

      setDXName(depth_stencil, "MainDepthBuffer");
      setDXName(depth_stencil_view, "MainDepthBufferView");

      // -----------------------------------------
      // Create a resource view so we can use the data in a shader
      // To read stencil 
      // https://stackoverflow.com/questions/34601325/directx11-read-stencil-bit-from-compute-shader
      D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
      ZeroMemory(&srv_desc, sizeof(srv_desc));
      srv_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
      srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
      srv_desc.Texture2D.MipLevels = desc.MipLevels;
      hr = device->CreateShaderResourceView(depth_stencil, &srv_desc, &depth_srv);
      if (FAILED(hr))
          return false;
      setDXName(depth_srv, "SwapChain.Z.srv");
  }

  {
      D3D11_TEXTURE2D_DESC desc = {};
      desc.Width = IR_SIZE;
      desc.Height = IR_SIZE;
      desc.MipLevels = 1;
      desc.ArraySize = 1;
      desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
      desc.SampleDesc.Count = 1;
      desc.SampleDesc.Quality = 0;
      desc.Usage = D3D11_USAGE_DEFAULT;
      desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
      desc.CPUAccessFlags = 0;
      desc.MiscFlags = 0;
      hr = device->CreateTexture2D(&desc, nullptr, &depth_stencil_irradiance);
      if (FAILED(hr))
          return false;

      // To use the previous texture as a zbuffer, I need a depth stencil VIEW
      D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
      descDSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
      descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
      descDSV.Texture2D.MipSlice = 0;
      hr = device->CreateDepthStencilView(depth_stencil_irradiance, &descDSV, &depth_stencil_view_irradiance);
      if (FAILED(hr))
          return false;

      setDXName(depth_stencil_irradiance, "MainDepthBufferIrradiance");
      setDXName(depth_stencil_view_irradiance, "MainDepthBufferViewIrradiance");

      // -----------------------------------------
      // Create a resource view so we can use the data in a shader
      // To read stencil 
      // https://stackoverflow.com/questions/34601325/directx11-read-stencil-bit-from-compute-shader
      D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
      ZeroMemory(&srv_desc, sizeof(srv_desc));
      srv_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
      srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
      srv_desc.Texture2D.MipLevels = desc.MipLevels;
      hr = device->CreateShaderResourceView(depth_stencil_irradiance, &srv_desc, &depth_srv_irradiance);
      if (FAILED(hr))
          return false;
      setDXName(depth_srv_irradiance, "SwapChain.Z.Irradiance.srv");
  }

  return true;
}

bool CRender::create(HWND hWnd, bool fullscreen, bool vsync) {
  
  this->fullscreen = fullscreen;
  this->vsync = vsync;

  if (!createDevice(hWnd))
    return false;
  if (!createDepthBuffer())
    return false;
  if (!createRenderUtils())
    return false;

  activateDefaultRenderState();

  rmt_BindD3D11(device, ctx);

  return true;
}

void CRender::activateBackBuffer() {

  PROFILE_FUNCTION("activateBackBuffer");
  assert(render_target_view);
  assert(depth_stencil_view);
  assert(ctx);

  // Start rendering in the backbuffer and the associated zbuffer
  ctx->OMSetRenderTargets(1, &render_target_view, depth_stencil_view);

  // Setup the viewport
  D3D11_VIEWPORT vp;
  vp.Width = (FLOAT)width;
  vp.Height = (FLOAT)height;
  vp.MinDepth = 0.0f;
  vp.MaxDepth = 1.0f;
  vp.TopLeftX = 0;
  vp.TopLeftY = 0;
  ctx->RSSetViewports(1, &vp);

}

void CRender::destroy() {

  rmt_UnbindD3D11();

  destroyRenderUtils();
  SAFE_RELEASE(depth_stencil_view);
  SAFE_RELEASE(depth_stencil_view_irradiance);
  SAFE_RELEASE(depth_stencil);
  SAFE_RELEASE(depth_stencil_irradiance);
  SAFE_RELEASE(render_target_view);
  SAFE_RELEASE(swap_chain);

  ctx->ClearState();
  // ctx->Flush();

  SAFE_RELEASE(ctx);
 
  // https://gamedev.net/forums/topic/607886-dx11-memory-leeks-that-i-cant-get-ridoff/4845309/
  // Call twice could be an option to avoid some memory leaks
  SAFE_RELEASE(device);

#ifdef _DEBUG
  // debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
#endif
}

void CRender::swapFrames() {

    PROFILE_FUNCTION("swapFrames");

    if (vsync) {
        // Lock to screen refresh rate.
        swap_chain->Present(1, 0);
    }
    else {
        // Present as fast as possible.
        swap_chain->Present(0, 0);
    }
}

void CRender::setFullScreenMode(bool mode)
{
    swap_chain->SetFullscreenState(mode, nullptr);
}

void CRender::toggleFullScreen()
{
    fullscreen = !fullscreen;
    swap_chain->SetFullscreenState(fullscreen, nullptr);
}