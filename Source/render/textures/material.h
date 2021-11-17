#pragma once
#include "components/common/comp_buffers.h"
#include "render/render_channel.h"

class CPipeline;
class CTexture;
class CPhysicalMaterial;
struct TCompBuffers;

std::string addSuffixBeforeExtension(const std::string& base, const std::string& new_suffix);

// Static texture which we load 
class CMaterial : public IResource {

  const CPipelineState* pipeline = nullptr;
  
  const CTexture* albedo			= nullptr;
  const CTexture* normal			= nullptr;
  const CTexture* combined			= nullptr; // AO, Rough, Metal
  const CTexture* emissive			= nullptr;
  const CTexture* height			= nullptr;
  const CPhysicalMaterial* phys_mat = nullptr;

  const CMaterial* material_shadows = nullptr;
  const CMaterial* material_skin = nullptr;
  const CMaterial* material_instanced = nullptr;
  
  int   priority = 0;
  eRenderChannel render_channel = eRenderChannel::RC_SOLID;

  TCompBuffers comp_buffers;
  CtesMaterial ctes;
  CBaseShaderCte* gpu_ctes = nullptr;

  bool  uses_instanced = false;
  bool  uses_skin = false;
  bool  render_emissive = true;
  bool  render_baked_ao = true;
  mutable bool ctes_dirty = false;

  // Optimization for dx11
  ID3D11ShaderResourceView* shader_resource_views[TS_NUM_SLOTS_PER_MATERIAL];

  void cacheSRVS();
  void activateTextures(int slot_base) const;
  void setCtesDirty() const { ctes_dirty = true; };

public:

  bool createFromJson(const json& j);
  void activate() const;
  void destroy();

  bool renderInMenu() const;

  bool usesSkin() const { return uses_skin; }
  bool usesInstanced() const { return uses_instanced; }
  eRenderChannel getRenderChannel() const { return render_channel; }
  const CMaterial* getShadowsMaterial() const { return material_shadows; }
  const CMaterial* getSkinMaterial() const { return material_skin; }
  const CMaterial* getInstancedMaterial() const { return material_instanced; }
  const CPhysicalMaterial* getPhysicalMaterial() const { return phys_mat; }
  int getPriority() const { return priority; }
  float getEmissive() const { return ctes.material_emissive_factor; };

  void activateBuffers( TCompBuffers* buffers ) const;
  void setAlbedo(const CTexture* new_albedo);
  void setEmissive(float value);
};

