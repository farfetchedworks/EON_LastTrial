#include "mcv_platform.h"
#include "material.h"
#include "render/render_manager.h"
#include "components/common/comp_buffers.h"
#include "render/compute/gpu_buffer.h"
#include "audio/physical_material.h"
#include "entity/entity_parser.h"
#include "render/shaders/shader_ctes_manager.h"

/// -----------------------------------
static NamedValues<eRenderChannel>::TEntry render_channel_entries[] = {
  {eRenderChannel::RC_SOLID, "solid"},
  {eRenderChannel::RC_SOLID_STENCIL, "solid_stencil"},
  {eRenderChannel::RC_SHADOW_CASTERS, "shadows"},
  {eRenderChannel::RC_SHADOW_CASTERS_DYNAMIC, "dynamic_shadows"},
  {eRenderChannel::RC_DISTORSIONS, "distorsion"},
  {eRenderChannel::RC_WATER, "water"},
  {eRenderChannel::RC_EMISSIVE, "emissive"},
  {eRenderChannel::RC_TRANSPARENT, "transparent"},
  {eRenderChannel::RC_DECALS, "decals"},
  {eRenderChannel::RC_UI, "ui"},
  {eRenderChannel::RC_DEBUG, "debug"},
};
NamedValues<eRenderChannel> render_channel_names(render_channel_entries, sizeof(render_channel_entries) / sizeof(NamedValues<eRenderChannel>::TEntry));

/// -----------------------------------
class CMaterialResourceType : public CResourceType {
public:
  const char* getExtension(int idx) const { return ".mat"; }
  const char* getName() const { return "Material"; }
  IResource* create(const std::string& name) const {

    json j = loadJson(name);

    // Right now, there is just one type of material
    CMaterial* material = new CMaterial();
    material->setResourceName(name);
    TFileContext file_ctx(name);
    bool is_ok = material->createFromJson(j);
    assert(is_ok);
    return material;
  }
};

void from_json(const json& j, CtesMaterial& ct) {
    ct.material_emissive_factor = j.value("material_emissive_factor", 1.f);
}

// -----------------------------------------
template<>
CResourceType* getClassResourceType<CMaterial>() {
  static CMaterialResourceType factory;
  return &factory;
}

std::string addSuffixBeforeExtension(const std::string& base, const std::string& new_suffix) {
  std::string::size_type idx = base.find_last_of(".");
  assert(idx != std::string::npos);
  std::string name_without_extension = base.substr(0, idx);
  name_without_extension += new_suffix;
  name_without_extension += base.substr(idx);
  return name_without_extension;
}

// -----------------------------------------
bool CMaterial::createFromJson(const json& j) {

  std::string pipe_name = j.value("pipeline", "pbr_gbuffer.pipeline");
  pipeline = Resources.get(pipe_name)->as<CPipelineState>();

  priority = j.value("priority", priority);

  std::string albedo_name = j.value("albedo", "data/textures/missing.dds");
  albedo = Resources.get(albedo_name)->as<CTexture>();

  std::string normal_name = j.value("normal", "data/textures/null_normal.dds");
  normal = Resources.get(normal_name)->as<CTexture>();

  std::string height_name = j.value("height", "data/textures/black.dds");
  height = Resources.get(height_name)->as<CTexture>();

  std::string combined_name = j.value("combined", "data/textures/combined_default.dds");
  combined = Resources.get(combined_name)->as<CTexture>();

  std::string emissive_name = j.value("emissive", "data/textures/black.dds");
  emissive = Resources.get(emissive_name)->as<CTexture>();

  std::string physmat_name = j.value("surface_type", "Gravel");
  phys_mat = new CPhysicalMaterial(physmat_name);

  if (j.count("render_channel")) {
    std::string render_channel_txt = j["render_channel"];
    render_channel = render_channel_names.valueOf(render_channel_txt.c_str());
  }

  // -------------------------------------------------
  const json& pipe_cfg = pipeline->getConfig();
  bool skin_supported = pipe_cfg.count("add_skin_support");
  if (skin_supported) {
    json jnew = j;
    jnew["pipeline"] = addSuffixBeforeExtension(pipe_name, "_skin");
    CMaterial* mat_new = new CMaterial();
    bool is_ok = mat_new->createFromJson(jnew);
    std::string name_new = addSuffixBeforeExtension(getName(), "_skin");
    Resources.registerResource(mat_new, name_new, getClassResourceType<CMaterial>());
    assert(is_ok);
    material_skin = mat_new;
  }

  bool instanced_supported = pipe_cfg.count("add_instanced_support");
  if (instanced_supported) {
    json jnew = j;
    jnew["pipeline"] = addSuffixBeforeExtension(pipe_name, "_instanced");
    CMaterial* mat_new = new CMaterial();
    bool is_ok = mat_new->createFromJson(jnew);
    std::string name_new = addSuffixBeforeExtension(getName(), "_instanced");
    Resources.registerResource(mat_new, name_new, getClassResourceType<CMaterial>());
    assert(is_ok);
    material_instanced = mat_new;
  }

  uses_skin = j.value("uses_skin", pipeline->useSkin());
  uses_instanced = j.value("uses_instanced", pipeline->useInstanced());

  bool casts_shadows = j.value("casts_shadows", true);
  if (casts_shadows) {
    bool is_dynamic = j.value("is_dynamic", uses_skin);
    std::string shadows_mat_name = j.value("shadows_mat", 
        is_dynamic ? "data/materials/shadow_caster_dynamic.mat" : "data/materials/shadow_caster.mat");
    material_shadows = Resources.get(shadows_mat_name)->as<CMaterial>();
    if (pipeline->useSkin())
      material_shadows = material_shadows->material_skin;
    if(pipeline->useInstanced())
      material_shadows = material_shadows->material_instanced;
  }

  cacheSRVS();

  if (!ShaderCtesManager.isDefined("CtesMaterial")) {
      ShaderCtesManager.registerStruct("CtesMaterial", 
          CShaderCtesManager::makeFactory<CtesMaterial>(CB_SLOT_MATERIAL));
  }

  bool usesCtes = false;

  if (j.count("buffers")) {
    TEntityParseContext ctx;
    const json jCb = j["buffers"];
    comp_buffers.load(jCb, ctx);
    
    if (jCb.count("CtesMaterial")) {
        gpu_ctes = comp_buffers.getCteByName("CtesMaterial");
        from_json(jCb["CtesMaterial"], ctes);
        usesCtes = true;
    }
  }

  // create by default
  if (!usesCtes) {
      gpu_ctes = ShaderCtesManager.createCte("CtesMaterial", j);
      from_json(j, ctes);
  }

  assert(gpu_ctes->size() == sizeof(CtesMaterial));
  gpu_ctes->updateFromCPU(&ctes);
  return true;
}

void CMaterial::setAlbedo(const CTexture* new_albedo) {
  albedo = new_albedo;
  cacheSRVS();
}

void CMaterial::setEmissive(float value) {
    ctes.material_emissive_factor = value;
    gpu_ctes->updateFromCPU(&ctes);
}

// Call this funcion any time a texture is hot-reloaded or for example if the background
// texture is updated, just when any srv of a texture is updated.
void CMaterial::cacheSRVS()
{
  // Clean the array
  for (int i = 0; i < TS_NUM_SLOTS_PER_MATERIAL; ++i)
    shader_resource_views[i] = nullptr;
  // Fill with the srv's of my textures
  if (albedo)
    shader_resource_views[TS_ALBEDO] = albedo->getShaderResourceView();
  if(normal)
    shader_resource_views[TS_NORMAL] = normal->getShaderResourceView();
  if(combined)
    shader_resource_views[TS_COMBINED] = combined->getShaderResourceView();
  if(emissive)
    shader_resource_views[TS_EMISSIVE] = emissive->getShaderResourceView();
  if (height)
    shader_resource_views[TS_HEIGHT] = height->getShaderResourceView();
}

void CMaterial::activateTextures(int slot_base) const
{
  Render.ctx->PSSetShaderResources(slot_base, TS_NUM_SLOTS_PER_MATERIAL, shader_resource_views);
}

void CMaterial::activate() const {
  if (pipeline)
    pipeline->activate();

  if (gpu_ctes) {
    gpu_ctes->activate();
  }

  assert(TS_ALBEDO == 0);
  activateTextures(TS_ALBEDO);
}

void CMaterial::destroy() {
  
  pipeline = nullptr;
  albedo = nullptr;
  normal = nullptr;
  combined = nullptr;
  emissive = nullptr;
  height = nullptr;

  auto ctes = comp_buffers.getCteByName("TMaterialCtes");
  SAFE_DESTROY(ctes);
  SAFE_DESTROY(gpu_ctes);
}

bool CMaterial::renderInMenu() const {

  if (ImGui::TreeNode("Textures..."))
  {
    if (albedo)
      albedo->renderInMenu();
    if (normal)
      normal->renderInMenu();
    if (combined)
        combined->renderInMenu();
    if (emissive)
      emissive->renderInMenu();
    if (height)
        height->renderInMenu();
    ImGui::TreePop();
  }

  render_channel_names.debugInMenu("Render Channel", *(eRenderChannel*)&render_channel);

  if( pipeline )
    ImGui::Text("Pipeline: %s", pipeline->getName().c_str());

  if (uses_skin)
    ImGui::Text("Use Skin");
  if (uses_instanced)
    ImGui::Text("Use Instanced");

  if (ImGui::DragInt("Priority", (int*)&priority, 0.01f, -5, 5))
    RenderManager.setDirty();

  ImGui::Text("RenderChannel: %d", render_channel);
  ImGui::Separator();
  ImGui::Checkbox("Use Emissive", &(bool&)render_emissive);
  ImGui::Checkbox("Use Baked AO", &(bool&)render_baked_ao);

  if (material_shadows)
    ImGui::Text("Shadows: %s", material_shadows->getName().c_str());
  if (material_skin)
    ImGui::Text("Shadows Skin: %s", material_skin->getName().c_str());
  if (material_instanced)
    ImGui::Text("Shadows Instanced: %s", material_instanced->getName().c_str());
  if (phys_mat)
    ImGui::Text("Physical Material: %s", phys_mat->getName());

  if (ImGui::DragFloat("Emissive factor", &(float&)ctes.material_emissive_factor, 0.01f, 0.f, 30.f))
      gpu_ctes->updateFromCPU(&ctes);

  return false;
}

void CMaterial::activateBuffers(TCompBuffers* c_buffers) const {
  const json& jcfg = pipeline->getConfig();
  if (!jcfg.count("gpu_buffers"))
    return;

  for (auto it : jcfg["gpu_buffers"].items()) {
    const std::string& name = it.key();
    int slot = it.value().get<int>();
    // Need to get remove the hardcoded value instances & the 0
    const CGPUBuffer* gpu_buffer = c_buffers->getBufferByName(name.c_str());
    assert(gpu_buffer);
    gpu_buffer->activate(slot);
  }

}
