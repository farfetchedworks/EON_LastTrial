#pragma once

class CRenderToTexture;

// Static texture which we load 
class CTexture : public IResource {

protected:

  // The texture object in d3d
  ID3D11Resource* texture = nullptr;

  // To be able to use the texture inside any shader, we need a shader resource view
  ID3D11ShaderResourceView*  shader_resource_view = nullptr;     // srv
  ID3D11UnorderedAccessView* uav = nullptr;

public:

  bool createFromFile(const std::string& filename);
  void activate(int nslot) const;
  void activateVS(int nslot) const;
  void activateCS(int nslot) const;
  void activateCSasUAV(int nslot) const;
  static void deactivate(int nslot);
  static void deactivateVS(int nslot);
  static void deactivateCS(int nslot);
  void destroy();
  void copyToCPU(void* buffer, int width, int height, int num_faces = 1);
  void update(void* buffer, int width, int height, int pixel_size);
  bool renderInMenu() const;

  void setDXParams(ID3D11Texture2D* new_texture, ID3D11ShaderResourceView* new_srv);

  ID3D11ShaderResourceView* getShaderResourceView() const { return shader_resource_view; }
  ID3D11UnorderedAccessView* getUAV() const { return uav; }
  ID3D11Resource* getDXResource() { return texture; }

  // Create a new texture from params
  enum eCreateOptions {
    CREATE_STATIC
  , CREATE_DYNAMIC
  , CREATE_STAGING
  , CREATE_RENDER_TARGET
  , CREATE_RENDER_TARGET_UAV
  , CREATE_WITH_COMPUTE_SUPPORT
  };

  bool create(const char* name, int new_xres, int new_yres, DXGI_FORMAT new_color_format, int create_options = CREATE_STATIC, bool is_array = false, int array_size = 1);
  void copyFromResource(CTexture* texture);
  bool updateFromIYUV(const uint8_t* new_data, size_t data_size);

  /* 
    Copy texture into render target using a specific shader
    - Returns the previous render target
  */
  CRenderToTexture* blit(CRenderToTexture* fbo, const CPipelineState* pipeline);
};

