#include "mcv_platform.h"
#include "texture.h"
#include "render_to_texture.h"
#include "DirectXTK/DDSTextureLoader.h"
#include "comdef.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "utils/stb_image_write.h"

class CTextureResourceType : public CResourceType {
public:
  const char* getExtension(int idx) const { return ".dds"; }
  const char* getName() const { return "Texture"; }
  IResource* create(const std::string& name) const {
    CTexture* texture = new CTexture();
    bool is_ok = texture->createFromFile(name);
    assert(is_ok);
    return texture;
  }
};

// -----------------------------------------
template<>
CResourceType* getClassResourceType<CTexture>() {
  static CTextureResourceType factory;
  return &factory;
}

void CTexture::setDXParams(ID3D11Texture2D* new_texture, ID3D11ShaderResourceView* new_srv) {
  //xres = new_xres;
  //yres = new_yres;
  texture = new_texture;
  shader_resource_view = new_srv;
  new_texture->AddRef();
  new_srv->AddRef();
}

void CTexture::copyToCPU(void* buffer, int width, int height, int num_faces)
{
    size_t Height = height, Width = width;

    for (size_t i = 0; i < Width * Height * 4 * num_faces; ++i) {
        ((float*)buffer)[i] = 0.f;
    }

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = Render.ctx->Map(texture, 0, D3D11_MAP_READ, 0, &mappedResource);
    if (FAILED(hr)) {
        _com_error err(hr);
        LPCTSTR errMsg = err.ErrorMessage();
        fatal("error: Couldn't map texture 2D: %s\n", errMsg);
        return;
    }

    //static int num = 0;

    int const BytesPerPixel = sizeof(FLOAT) * 4;

    for (int face = 0; face < num_faces; ++face) {
        for (size_t i = Height * face; i < Height * (face + 1); ++i)
        {
            std::memcpy(
                (byte*)buffer + Width * BytesPerPixel * i,
                (byte*)mappedResource.pData + mappedResource.RowPitch * i,
                Width * BytesPerPixel);
        }

        //std::string name = std::to_string(num) + "_test" + std::to_string(face) + ".hdr";
        //stbi_write_hdr(name.c_str(), width, height, 4, (const float*)((byte*)buffer + Width * Height * BytesPerPixel * face));
    }

    // size of the texture's data defined by the length of a single row of data * the total number of rows (texture height)
    // buffer = new FLOAT[mappedResource.RowPitch * height];
    // memcpy(ºbuffer, mappedResource.pData, mappedResource.RowPitch * (size_t)height);
    //subresources[i].SysMemPitch = mappedResource.RowPitch;
    //subresources[i].SysMemSlicePitch = mappedSubResource.DepthPitch;

    //num++;

    Render.ctx->Unmap(texture, 0);
}

void CTexture::update(void* buffer, int width, int height, int pixel_size)
{
    ID3D11Texture2D* tex2D = (ID3D11Texture2D*)texture;
    D3D11_TEXTURE2D_DESC desc;
    tex2D->GetDesc(&desc);

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = Render.ctx->Map(texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(hr)) {
        _com_error err(hr);
        LPCTSTR errMsg = err.ErrorMessage();
        fatal("Could update texture 2D: %s\n", errMsg);
        return;
    }

    const uint32_t srcPitch = pixel_size * width;
    BYTE* textureData = reinterpret_cast<BYTE*>(mappedResource.pData);
    const BYTE* srcData = reinterpret_cast<BYTE*>(buffer);
    for (int i = 0; i < height; ++i)
    {
        // Copy the texture data for a single row
        memcpy(textureData, srcData, srcPitch);

        // Advance the pointers
        textureData += mappedResource.RowPitch;
        srcData += srcPitch;
    }

    Render.ctx->Unmap(texture, 0);
}

// -----------------------------------------
void CTexture::destroy() {
  SAFE_RELEASE(shader_resource_view);
  SAFE_RELEASE(uav);
  SAFE_RELEASE(texture);
}

// ------------------------------------------------
bool CTexture::create(
  const char* name
  ,int nxres
  , int nyres
  , DXGI_FORMAT nformat
  , int options
  , bool is_array
  , int array_size
) {

  this->name = name;

  D3D11_TEXTURE2D_DESC desc;
  ZeroMemory(&desc, sizeof(desc));
  desc.Width = nxres;
  desc.Height = nyres;
  desc.MipLevels = 1;
  desc.ArraySize = array_size;
  desc.Format = nformat;
  desc.SampleDesc.Count = 1;
  desc.SampleDesc.Quality = 0;
  desc.Usage = D3D11_USAGE_DEFAULT;
  desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  desc.CPUAccessFlags = 0;
  desc.MiscFlags = /*is_cubemap ? D3D11_RESOURCE_MISC_TEXTURECUBE : */0;

  if (options == CREATE_DYNAMIC) {
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;

  }
  else if (options == CREATE_RENDER_TARGET) {
    desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
  }  
  else if (options == CREATE_RENDER_TARGET_UAV) {
    desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
    desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;

    if (desc.Format == DXGI_FORMAT_R8G8B8A8_UNORM)
        desc.Format = DXGI_FORMAT_R8G8B8A8_TYPELESS;
    if (desc.Format == DXGI_FORMAT_R32_FLOAT)
        desc.Format = DXGI_FORMAT_R32_TYPELESS;
  }
  else if (options == CREATE_WITH_COMPUTE_SUPPORT) {
    desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
  }
  else if (options == CREATE_STAGING) {
    desc.BindFlags = 0;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.CPUAccessFlags |= D3D11_CPU_ACCESS_READ;
  }
  else {
    assert(options == CREATE_STATIC);
  }

  // Initialize with zeroes
  int channels_size = numChannelsInFormat(nformat);
  int bytes_size = numBytesPerChannelInFormat(nformat);
  //BYTE* buf = (BYTE*)malloc(nxres * nyres * channels_size * bytes_size);

  //for (int i = 0; i < nxres * nyres * channels_size * bytes_size; i++)
  //    buf[i] = 0;

  //D3D11_SUBRESOURCE_DATA data;
  //data.pSysMem = (void*)buf;
  //data.SysMemPitch = nxres * channels_size *  bytes_size;
  //data.SysMemSlicePitch = 0;

  ID3D11Texture2D* tex2d = nullptr;
  HRESULT hr = Render.device->CreateTexture2D(&desc, nullptr, &tex2d);
  if (FAILED(hr))
    return false;
  texture = tex2d;
  setDXName(texture, getName().c_str());

  // -----------------------------------------
  // Create a resource view so we can use the data in a shader
  D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
  ZeroMemory(&srv_desc, sizeof(srv_desc));
  srv_desc.Format = nformat;
  srv_desc.ViewDimension = is_array ? D3D11_SRV_DIMENSION_TEXTURE2DARRAY : D3D11_SRV_DIMENSION_TEXTURE2D;
  if (is_array) {
      srv_desc.Texture2DArray.ArraySize = array_size;
      srv_desc.Texture2DArray.FirstArraySlice = 0;
      srv_desc.Texture2DArray.MipLevels = desc.MipLevels;
      srv_desc.Texture2DArray.MostDetailedMip = 0;
  }
  else {
      srv_desc.Texture2D.MipLevels = desc.MipLevels;
  }

  hr = Render.device->CreateShaderResourceView(texture, &srv_desc, &shader_resource_view);
  if (FAILED(hr))
    return false;
  setDXName(shader_resource_view, getName().c_str());

  // -----------------------------------------
  // If an UAV object has been requested, create it
  if (desc.BindFlags & D3D11_BIND_UNORDERED_ACCESS) {
    D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
    uav_desc.ViewDimension = is_array ? D3D11_UAV_DIMENSION_TEXTURE2DARRAY : D3D11_UAV_DIMENSION_TEXTURE2D;
    uav_desc.Texture2D.MipSlice = 0;

    if (options == CREATE_RENDER_TARGET_UAV && nformat == DXGI_FORMAT_R8G8B8A8_UNORM) {
        uav_desc.Format = DXGI_FORMAT_R32_UINT;
    } else 
    if (options == CREATE_RENDER_TARGET_UAV && nformat == DXGI_FORMAT_R32_FLOAT) {
        uav_desc.Format = DXGI_FORMAT_R32_FLOAT;
    } else {
        uav_desc.Format = DXGI_FORMAT_UNKNOWN;
    }

    hr = Render.device->CreateUnorderedAccessView(texture, &uav_desc, &uav);
    if (FAILED(hr))
      return false;
    setDXName(uav, getName().c_str());
  }

  return true;
}

CRenderToTexture* CTexture::blit(CRenderToTexture* fbo, const CPipelineState* pipeline)
{
    auto prev_rt = fbo->activateRT();
    pipeline->activate();
    auto* mesh = Resources.get("unit_quad_xy.mesh")->as<CMesh>();
    mesh->activate();
    activate(TS_ALBEDO);
    mesh->render();
    deactivate(TS_ALBEDO);
    return prev_rt;
}

void CTexture::copyFromResource(CTexture* texture_to_copy)
{
  Render.ctx->CopyResource(getDXResource(), texture_to_copy->getDXResource());
}

void CTexture::activate(int nslot) const {
  Render.ctx->PSSetShaderResources(nslot, 1, &shader_resource_view);
}

void CTexture::activateVS(int nslot) const
{
  Render.ctx->VSSetShaderResources(nslot, 1, &shader_resource_view);
}

void CTexture::activateCS(int nslot) const
{
  Render.ctx->CSSetShaderResources(nslot, 1, &shader_resource_view);
}

void CTexture::activateCSasUAV(int nslot) const
{
  UINT zero = 0;
  Render.ctx->CSSetUnorderedAccessViews(nslot, 1, &uav, &zero);
}

void CTexture::deactivate(int nslot) {
  ID3D11ShaderResourceView* null_srv = nullptr;
  Render.ctx->PSSetShaderResources(nslot, 1, &null_srv);
}

void CTexture::deactivateVS(int nslot)
{
  ID3D11ShaderResourceView* null_srv = nullptr;
  Render.ctx->VSSetShaderResources(nslot, 1, &null_srv);
}

void CTexture::deactivateCS(int nslot)
{
  ID3D11ShaderResourceView* null_srv = nullptr;
  Render.ctx->CSSetShaderResources(nslot, 1, &null_srv);
}

bool CTexture::createFromFile(const std::string& filename) {

  wchar_t wFilename[MAX_PATH];
  mbstowcs(wFilename, filename.c_str(), MAX_PATH);

  HRESULT hr = DirectX::CreateDDSTextureFromFile(
    Render.device,
    wFilename,
    &texture,
    &shader_resource_view
  );

  if (FAILED(hr)) {
    fatal("Failed to load texture %s. HResult=%08x\n", filename.c_str(), hr);
    return false;
  }

  return true;
}

bool CTexture::renderInMenu() const {

  D3D11_RESOURCE_DIMENSION ResourceDimension;
  texture->GetType(&ResourceDimension);

  if (ResourceDimension == D3D11_RESOURCE_DIMENSION_TEXTURE2D) {
    ID3D11Texture2D* tex2D = (ID3D11Texture2D*)texture;
    D3D11_TEXTURE2D_DESC desc;
    tex2D->GetDesc(&desc);
    ImGui::Text("%dx%d %s", desc.Width, desc.Height, getName().c_str());
  }

  ImGui::Image(shader_resource_view, ImVec2(256, 256));

  return false;
}

bool CTexture::updateFromIYUV(const uint8_t* data, size_t data_size) {
  assert(data);
  D3D11_MAPPED_SUBRESOURCE ms;
  HRESULT hr = Render.ctx->Map(texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
  if (FAILED(hr))
    return false;

  // Read resolution and format from the texture
  ID3D11Texture2D* tex2d = static_cast<ID3D11Texture2D*>(texture);
  D3D11_TEXTURE2D_DESC desc;
  tex2d->GetDesc(&desc);
  int xres = desc.Width;
  int yres = desc.Height;
  DXGI_FORMAT format = desc.Format;

  uint32_t bytes_per_texel = 1;
  assert(format == DXGI_FORMAT_R8_UNORM);
  assert(data_size == xres * yres * 3 / 4);

  const uint8_t* src = data;
  uint8_t* dst = (uint8_t*)ms.pData;

  // Copy the Y lines
  uint32_t nlines = yres / 2;
  uint32_t bytes_per_row = xres * bytes_per_texel;
  for (uint32_t y = 0; y < nlines; ++y) {
    memcpy(dst, src, bytes_per_row);
    src += bytes_per_row;
    dst += ms.RowPitch;
  }

  // Now the U and V lines, need to add Width/2 pixels of padding between each line
  uint32_t uv_bytes_per_row = bytes_per_row / 2;
  for (uint32_t y = 0; y < nlines; ++y) {
    memcpy(dst, src, uv_bytes_per_row);
    src += uv_bytes_per_row;
    dst += ms.RowPitch;
  }

  Render.ctx->Unmap(texture, 0);
  return true;
}
