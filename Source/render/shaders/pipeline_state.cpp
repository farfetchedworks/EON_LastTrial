#include "mcv_platform.h"
#include "pipeline_state.h"
#include "render/render.h"

class CPipelineStateResourceType : public CResourceType {
public:
  const char* getExtension(int idx) const { return ".pipeline"; }
  const char* getName() const { return "Pipeline"; }
  IResource* create(const std::string& name) const {
    fatal("PipelineStates must be defined in the data/pipelines.json. Failed to create resource %s", name.c_str());
    return nullptr;
  }
};

// -----------------------------------------
template<>
CResourceType* getClassResourceType<CPipelineState>() {
  static CPipelineStateResourceType factory;
  return &factory;
}

const CPipelineState* CPipelineState::curr_pipeline = nullptr;

void CPipelineState::activate() const {

  if (curr_pipeline == this)
    return;
  curr_pipeline = this;

  vs.activate();
  ps.activate();
  activateRSConfig(rs_config);
  activateZConfig(z_config);
  activateBlendConfig(blend_config);
  if (env)
    env->activate(TS_ENVIRONMENT);
  if (env_prefiltered)
    env_prefiltered->activate(TS_ENVIRONMENT_PF);  
  if (brdf_lut)
    brdf_lut->activate(TS_BRDF_LUT);
  if (noise)
    noise->activate(TS_NOISE);
}

void CPipelineState::destroy() {
  vs.destroy();
  ps.destroy();
  env = nullptr;
  env_prefiltered = nullptr;
  noise = nullptr;
}

bool CPipelineState::create(const json& j) {
  // Copy the full json s
  jcfg = j;

  std::string vs_filename = j.value("vs_filename", "");
  std::string vertex_decl_name = j.value("vertex_decl", "");
  std::string vs_entry = j.value("vs_entry", "VS");

  std::string ps_filename = j.value("ps_filename", vs_filename);

  bool ps_is_null = j.count("ps_entry") && j["ps_entry"].is_null();

  bool is_ok = true;
  is_ok &= vs.create(vs_filename.c_str(), vs_entry.c_str(), vertex_decl_name.c_str());
  if (!ps_is_null) {
    std::string ps_entry = j.value("ps_entry", "PS");
    is_ok &= ps.create(ps_filename.c_str(), ps_entry.c_str());
  }

  rs_config = RSConfigFromString(j.value("rs", "default"));
  z_config = ZConfigFromString(j.value("z", "default"));
  blend_config = BlendConfigFromString(j.value("blend", "default"));

  if (j.count("env")) {
    std::string env_name = j["env"];
    env = Resources.get(env_name)->as<CTexture>();

    //std::string env_prefiltered_name = j["irradiance"];
    //env_prefiltered = Resources.get(env_prefiltered_name)->as<CTexture>();

    std::string brdf_name = j["brdf_lut"];
    brdf_lut = Resources.get(brdf_name)->as<CTexture>();
  }
  if (j.count("noise")) {
    std::string noise_name = j["noise"];
    noise = Resources.get(noise_name)->as<CTexture>();
  }

  use_instanced = j.value("use_instanced", use_instanced);
  use_skin = j.value("use_skin", use_skin);

  return is_ok;
}

bool CPipelineState::renderInMenu() const {

  // Remove the const
  ::renderInMenu((ZConfig&)z_config);
  ::renderInMenu((RSConfig&)rs_config);
  ::renderInMenu((BlendConfig&)blend_config);

  if (use_skin)
    ImGui::Text("Skin");
  if (use_instanced)
    ImGui::Text("Instanced");
  ImGui::Text("%s", jcfg.dump(2, ' ').c_str());
  return false;
}


