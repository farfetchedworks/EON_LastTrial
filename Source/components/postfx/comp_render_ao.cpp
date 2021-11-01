#include "mcv_platform.h"
#include "comp_render_ao.h"
#include "resources/resources.h"
#include "components/common/comp_camera.h"
#include "render/textures/render_to_texture.h"

// Using https://github.com/GameTechDev/ASSAO

DECL_OBJ_MANAGER("render_ao", TCompRenderAO);

// ---------------------
void TCompRenderAO::debugInMenu( ) {
  ImGui::Checkbox("Enabled", &enabled);

  ImGui::Text("Performance/quality settings:");

  // extension for "Lowest"
  int qualityLevelUI = settings.QualityLevel + 1;

  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.8f, 1.0f));
#ifdef INTEL_SSAO_ENABLE_ADAPTIVE_QUALITY
  ImGui::Combo("Quality level", &qualityLevelUI, "Lowest\0Low\0Medium\0High\0Highest (adaptive)\0\0");  // Combo using values packed in a single constant string (for really quick combo)
#else
  ImGui::Combo("Quality level", &qualityLevelUI, "Lowest\0Low\0Medium\0High\0\0");  // Combo using values packed in a single constant string (for really quick combo)
#endif
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Each quality level is roughly 2x more costly than the previous, except the Highest (adaptive) which is variable but, in general, above High");
  ImGui::PopStyleColor(1);

  // extension for "Lowest"
#ifdef INTEL_SSAO_ENABLE_ADAPTIVE_QUALITY
  settings.QualityLevel = std::clamp(qualityLevelUI - 1, -1, 3);
#else
  settings.QualityLevel = std::clamp(qualityLevelUI - 1, -1, 2);
#endif

  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f, 0.75f, 0.75f, 1.0f));

  if (settings.QualityLevel == 3)
  {
#ifdef INTEL_SSAO_ENABLE_ADAPTIVE_QUALITY
    ImGui::Indent();
    ImGui::SliderFloat("Adaptive quality target", &settings.AdaptiveQualityLimit, 0.0f, 1.0f, "%.3f");
    settings.AdaptiveQualityLimit = std::clamp(settings.AdaptiveQualityLimit, 0.0f, 1.0f);
    ImGui::Unindent();
#endif
  }

  if (settings.QualityLevel <= 0)
  {
    ImGui::InputInt("Simple blur passes (0-1)", &settings.BlurPassCount);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("For Low quality, only one optional simple blur pass can be applied (recommended); settings above 1 are ignored");
  }
  else
  {
    ImGui::InputInt("Smart blur passes (0-6)", &settings.BlurPassCount);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("The amount of edge-aware smart blur; each additional pass increases blur effect but adds to the cost");
  }
  settings.BlurPassCount = std::clamp(settings.BlurPassCount, 0, 6);

  ImGui::Separator();
  ImGui::Text("Visual settings:");
  ImGui::DragFloat("Effect radius", &settings.Radius, 0.01f, 0.0f, 10.0f);
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("World (viewspace) effect radius");
  ImGui::DragFloat("Occlusion multiplier", &settings.ShadowMultiplier, 0.02f, 0.0f, 5.0f);
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Effect strength");
  ImGui::DragFloat("Occlusion power curve", &settings.ShadowPower, 0.01f, 0.5f, 5.0f);
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("occlusion = pow( occlusion, value ) - changes the occlusion curve");
  ImGui::DragFloat("Fadeout distance from", &settings.FadeOutFrom, 1.0f, 0.0f, 100.f);
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Distance at which to start fading out the effect");
  ImGui::DragFloat("Fadeout distance to", &settings.FadeOutTo, 1.0f, 0.0f, 100.f);
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Distance at which to completely fade out the effect");
  ImGui::DragFloat("Sharpness", &settings.Sharpness, 0.01f, 0.0f, 1.0f);
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("How much to bleed over edges; 1: not at all, 0.5: half-half; 0.0: completely ignore edges");

  ImGui::Separator();
  ImGui::Text("Advanced visual settings:");
  ImGui::DragFloat("Detail occlusion multiplier", &settings.DetailShadowStrength, 0.02f, 0.0f, 5.0);
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Additional small radius / high detail occlusion; too much will create aliasing & temporal instability");
  ImGui::DragFloat("Horizon angle threshold", &settings.HorizonAngleThreshold, 0.01f, 0.0f, 1.0f);
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Reduces precision and tessellation related unwanted occlusion");
  ImGui::DragFloat("Occlusion max clamp", &settings.ShadowClamp, 0.01f, 0.0f, 1.0f);
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("occlusion = min( occlusion, value ) - limits the occlusion maximum");
  //ImGui::InputFloat( "Radius distance-based modifier",    &settings.RadiusDistanceScalingFunction   , 0.05f, 0.0f, 2 );
  //if( ImGui::IsItemHovered( ) ) ImGui::SetTooltip( "Used to modify ""Effect radius"" based on distance from the camera; for 1.0, effect world radius is constant (default);\nfor values smaller than 1.0, the effect radius will grow the more distant from the camera it is; if changed, ""Effect Radius"" often needs to be rebalanced as well" );

  settings.Radius = std::clamp(settings.Radius, 0.0f, 100.0f);
  settings.HorizonAngleThreshold = std::clamp(settings.HorizonAngleThreshold, 0.0f, 1.0f);
  settings.ShadowMultiplier = std::clamp(settings.ShadowMultiplier, 0.0f, 5.0f);
  settings.ShadowPower = std::clamp(settings.ShadowPower, 0.5f, 5.0f);
  settings.ShadowClamp = std::clamp(settings.ShadowClamp, 0.1f, 1.0f);
  settings.FadeOutFrom = std::clamp(settings.FadeOutFrom, 0.0f, 1000000.0f);
  settings.FadeOutTo = std::clamp(settings.FadeOutTo, 0.0f, 1000000.0f);
  settings.Sharpness = std::clamp(settings.Sharpness, 0.0f, 1.0f);
  settings.DetailShadowStrength = std::clamp(settings.DetailShadowStrength, 0.0f, 5.0f);
  //settings.RadiusDistanceScalingFunction    = vaMath::Clamp( settings.RadiusDistanceScalingFunction   , 0.1f, 1.0f        );

  ImGui::PopStyleColor(1);
}


void TCompRenderAO::load(const json& j, TEntityParseContext& ctx) {

  enabled = j.value("enabled", enabled);
  rt_output = new CRenderToTexture();

  // Read initial values for the intel settings
  settings.Radius = j.value("Radius", settings.Radius);
  settings.ShadowMultiplier = j.value("ShadowMultiplier", settings.ShadowMultiplier);
  settings.ShadowPower = j.value("ShadowPower", settings.ShadowPower);

  // Assign an unique name only once
  static int g_counter = 0;
  char rt_name[64];
  sprintf(rt_name, "AO_%02d", g_counter++);
  Resources.registerResource(rt_output, rt_name, getClassResourceType<CTexture>());

  prepareRenderTarget();
}

void TCompRenderAO::prepareRenderTarget()
{
  int screen_width = (int)Render.getWidth();
  int screen_height = (int)Render.getHeight();

  // All good, resolution matches
  if (rt_output && xres == screen_width && yres == screen_height)
    return;

  // Destroy previous resource internals.
  if (rt_output)
    rt_output->destroy();

  xres = screen_width;
  yres = screen_height;

  // Recreate rt, using the same name so it's not registered again.
  bool is_ok = rt_output->createRT(rt_output->getName().c_str(), xres, yres, DXGI_FORMAT_R8_UNORM, DXGI_FORMAT_UNKNOWN);
  assert(is_ok);
}

const CTexture* TCompRenderAO::compute(CTexture* normals, ASSAO_Effect* fx)
{
  if (!enabled)
    return nullptr;

  assert(normals);
  assert(fx);

  CGpuScope gpu_scope("AO");

  // Give support to rescale window size
  prepareRenderTarget();

  rt_output->activateRT();

  // My sibling component
  TCompCamera* c_camera = get<TCompCamera>();
  assert(c_camera);

  assao_inputs.ScissorRight = xres;
  assao_inputs.ScissorBottom = yres;
  assao_inputs.ViewportWidth = xres;
  assao_inputs.ViewportHeight = yres;
  assao_inputs.DepthSRV = Render.depth_srv;
  assao_inputs.OverrideOutputRTV = nullptr;
  assao_inputs.DeviceContext = Render.ctx;
  assao_inputs.NormalSRV = nullptr;
  assao_inputs.DrawOpaque = true;
  assao_inputs.MatricesRowMajorOrder = true;
  MAT44 view = c_camera->getView();

#ifdef INTEL_SSAO_ENABLE_NORMAL_WORLD_TO_VIEW_CONVERSION
  assao_inputs.NormalSRV = normals->getShaderResourceView();
  //view = view.Transpose();
  assao_inputs.NormalsWorldToViewspaceMatrix = *(ASSAO_Float4x4*)&view;
  //assao_inputs.NormalsWorldToViewspaceMatrix = *(ASSAO_Float4x4*)&MAT44::Identity;
#endif

  MAT44 proj = c_camera->getProjection();
  //proj = proj.Transpose();
  assao_inputs.ProjectionMatrix = *(ASSAO_Float4x4*)&proj;

  fx->Draw(settings, &assao_inputs);

  // Stop rendering into the rt_output
  CRenderToTexture::deactivate(1);

  return rt_output;
}
