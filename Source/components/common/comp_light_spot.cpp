#include "mcv_platform.h"
#include "comp_light_spot.h"
#include "comp_transform.h"
#include "render/draw_primitives.h"
#include "render/render_manager.h"
#include "render/render.h"
#include "components/render/comp_irradiance_cache.h"

DECL_OBJ_MANAGER("light_spot", TCompLightSpot)

extern CShaderCte<CtesLight> cte_light;

void TCompLightSpot::load(const json& j, TEntityParseContext& ctx) {
  TCompCamera::load(j, ctx);
  color = loadColor(j, "color");
  intensity = j.value("intensity", intensity);
  godrays_intensity = j.value("godrays_intensity", godrays_intensity);
  irr_intensity = j.value("irradiance_intensity", irr_intensity);
  enabled = j.value("enabled", enabled);
  cast_godrays = j.value("cast_godrays", cast_godrays);
  use_pattern = j.value("use_pattern", use_pattern);
  shadows_step = j.value("shadows_step", shadows_step);
  shadows_bias = j.value("shadows_bias", shadows_bias);

  if (j.count("pattern")) 
    pattern = Resources.get(j["pattern"])->as<CTexture>();

  // Check if we need to allocate a shadow map
  casts_shadows = j.value("casts_shadows", casts_shadows);
  if (casts_shadows) {
    auto shadowmap_fmt = readFormat(j, "shadows_fmt");    
    shadows_resolution = j.value("shadows_resolution", shadows_resolution);

    assert(shadows_resolution > 0);
    shadows_rt = new CRenderToTexture();
    cached_shadow_map = new CRenderToTexture();

    // Make a unique name to have the Resource Manager happy with the unique names for each resource
    char my_name[64];
    sprintf(my_name, "shadow_map_%08x", CHandle(this).asUnsigned());
    bool is_ok = shadows_rt->createRT(my_name, shadows_resolution, shadows_resolution, DXGI_FORMAT_UNKNOWN, shadowmap_fmt);
    sprintf(my_name, "cached_shadow_map_%08x", CHandle(this).asUnsigned());
    is_ok &= cached_shadow_map->createRT(my_name, shadows_resolution, shadows_resolution, DXGI_FORMAT_UNKNOWN, shadowmap_fmt);
    assert(is_ok);
  }

  shadows_enabled = casts_shadows;
}

void TCompLightSpot::onEntityCreated()
{
    h_transform = get<TCompTransform>();
}

void TCompLightSpot::onAllEntitiesCreated(const TMsgAllEntitiesCreated& msg)
{
    frames = 0;
}

void TCompLightSpot::update(float dt) {
  TCompTransform* c_transform = get<TCompTransform>();
  assert(c_transform);
  lookAt(c_transform->getPosition(), c_transform->getPosition() + c_transform->getForward());
}

void TCompLightSpot::debugInMenu() {
  bool changed = TCompCamera::innerDebugInMenu();

  if (changed)
      frames = 0;

  ImGui::Checkbox("Enabled", &enabled);
  ImGui::ColorEdit4("Color", &color.x);
  ImGui::DragFloat("Intensity", &intensity, 0.02f, 0.f, 50.f);
  ImGui::DragFloat("Shadow Step", &shadows_step, 0.0f, 0.f, 5.f);
  ImGui::DragFloat("Bias", &shadows_bias, 0.0001f, -0.01f, 0.01f);
  ImGui::Checkbox("Use pattern", &use_pattern);
  if(shadows_rt)
    ImGui::Checkbox("Shadows", &shadows_enabled);
  ImGui::Checkbox("Cast Godrays", &cast_godrays);
  if(cast_godrays)
      ImGui::DragFloat("Godrays Intensity", &godrays_intensity, 0.0f, 0.f, 10.f);
  
  ImGui::Image(shadows_rt->getZTexture()->getShaderResourceView(), ImVec2(256, 256));
  Resources.edit(&pattern);
}

void TCompLightSpot::renderDebug() {
#ifdef _DEBUG
  MAT44 inv_view_proj = getWorld();
  const CMesh* mesh = Resources.get("view_volume_wired.mesh")->as<CMesh>();
  for (int i = 1; i <= 10; ++i) {
    float scale_factor = (float)i / 10.f;
    MAT44 scale = MAT44::CreateScale(VEC3(1.0f, 1.0f, scale_factor));
    drawPrimitive( mesh, scale * inv_view_proj, Colors::Yellow);
  }
#endif
}

MAT44 TCompLightSpot::getWorld() const {
  return getViewProjection().Invert();
}

bool TCompLightSpot::activate() {

  bool skip = intensity == 0.0f || !enabled;

  // Don't skip light if default custom irradiance multiplier
  skip &= !(TCompIrradianceCache::RENDERING_ACTIVE && irr_intensity > 1.f);

  if (skip)
      return false;

  float irr_scale = TCompIrradianceCache::GLOBAL_IRRADIANCE_MULTIPLIER;

  // If rendering irradiance, apply global and custom multiplier
  //{
  //    if (TCompIrradianceCache::RENDERING_ACTIVE)
  //        irr_scale = irr_intensity;
  //}

  cte_light.light_color = color;
  cte_light.light_intensity = intensity * irr_scale;
  cte_light.light_position = getEye();
  cte_light.light_forward = getForward();
  cte_light.light_max_radius = getFar() * 0.9f;
  cte_light.light_shadows_step = shadows_step;
  cte_light.light_shadows_inv_resolution = 1.0f / (float)shadows_resolution;
  cte_light.light_shadows_step_over_resolution = shadows_step / (float)shadows_resolution;
  cte_light.light_shadows_bias = shadows_bias;
  cte_light.light_view_projection = getViewProjection();
  cte_light.light_view_projection_offset = cte_light.light_view_projection * MAT44::CreateScale(VEC3(0.5f, -0.5f, 1.0f))  * MAT44::CreateTranslation(VEC3(0.5f, 0.5f, 0));
  cte_light.light_godrays_intensity = godrays_intensity;
  cte_light.light_fov = fov_vertical_radians;
  cte_light.light_zfar = z_max;
  cte_light.light_use_pattern = (pattern && use_pattern) ? 1.f : 0.f;

  cte_light.updateFromCPU();
  
  if(pattern)
    pattern->activate(TS_LIGHT_PATTERN);

  if (shadows_rt) {
    assert(shadows_rt->getZTexture());
    shadows_rt->getZTexture()->activate(TS_LIGHT_SHADOW_MAP);
  }

  return true;
}

void TCompLightSpot::generateShadowMap() {

  if (!shadows_rt || !shadows_enabled)
    return;

  CHandle h_camera = CHandle(this).getOwner();
  
  CTexture::deactivate(TS_LIGHT_SHADOW_MAP);
  shadows_rt->activateRT();
  activateCamera(*this);

  // Render statics and store shadowmap
  if (frames < 2)
  {
      CGpuScope gpu_scope("shadowMapStatics");
      frames++;
      shadows_rt->clearZ();
      RenderManager.renderAll(eRenderChannel::RC_SHADOW_CASTERS, h_camera);
      copyZ(shadows_rt, cached_shadow_map); // Store shadowmap
  }
  // Render only dynamic entities
  else
  {
      CGpuScope gpu_scope("ShadowMapDynamics");

      CEntity* player = getEntityByName("player");
      if (player) {
          TCompTransform* c_trans_player = player->get<TCompTransform>();
          TCompTransform* c_trans_camera = h_transform;

          if (c_trans_player->distance(*c_trans_camera) > getFar()) {
              return;
          }
      }

      // Start from cached shadowmap
      copyZ(cached_shadow_map, shadows_rt);
      RenderManager.renderAll(eRenderChannel::RC_SHADOW_CASTERS_DYNAMIC, h_camera);
  }
}

void TCompLightSpot::copyZ(CRenderToTexture* src, CRenderToTexture* dst)
{
    assert(src && dst);
    dst->getZTexture()->copyFromResource(src->getZTexture());
}
