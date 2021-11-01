#include "mcv_platform.h"
#include "comp_camera.h"
#include "comp_transform.h"
#include "geometry/angular.h"

DECL_OBJ_MANAGER("camera", TCompCamera)

void TCompCamera::load(const json& j, TEntityParseContext& ctx) {

  z_min = j.value("z_min", z_min);
  z_max = j.value("z_max", z_max);

  if (j.value("ortho", false)) {
      float new_width = j.value("ortho_width", ortho_width);
      float new_height = j.value("ortho_height", ortho_height);
      float new_left = j.value("ortho_left", ortho_left);
      float new_top = j.value("ortho_top", ortho_top);
      bool centered = j.value("ortho_centered", ortho_centered);
      setOrthoParams(centered, new_left, new_width, new_top, new_height, z_min, z_max);
  }
  else {
      float fov_deg = rad2deg(getFov());
      fov_deg = j.value("fov", fov_deg);
      setProjectionParams(deg2rad(fov_deg), 1.0f, z_min, z_max);
  }
}

void TCompCamera::update(float dt) {
  TCompTransform* c_transform = get<TCompTransform>();
  assert(c_transform);
  lookAt(c_transform->getPosition(), c_transform->getPosition() + c_transform->getForward());
}

bool TCompCamera::innerDebugInMenu() {

    float fov_deg = rad2deg(getFov());
    float new_znear = getNear();
    float new_zfar = getFar();

    bool changed = false;

    changed |= ImGui::Checkbox("Is Ortho", &is_ortho);
    if (is_ortho) {
        bool keep_aspect_ratio = true;
        ImGui::Checkbox("Keep Ratio", &keep_aspect_ratio);
        changed |= ImGui::DragFloat("Width", &ortho_width, 0.1f, 0.1f, 512.0f);
        if (keep_aspect_ratio && changed) {
            ortho_height = ortho_width / aspect_ratio;
            ImGui::Text("Height: %f", ortho_height);
        }
        else
            changed |= ImGui::DragFloat("Height", &ortho_height, 0.1f, 0.1f, 512.0f);
        changed |= ImGui::Checkbox("Centered", &ortho_centered);
        if (!ortho_centered) {
            changed |= ImGui::DragFloat("Left", &ortho_left, 0.1f, 0.1f, 512.0f);
            changed |= ImGui::DragFloat("Top", &ortho_top, 0.1f, 0.1f, 512.0f);
        }
        // In Ortho cameras the znear is not clamped to positive values
        changed |= ImGui::DragFloat("Z Near", &new_znear, 0.001f, -100, 100.0f);
        changed |= ImGui::DragFloat("Z Far", &new_zfar, 1.0f, 2.0f, 3000.0f);
    }
    else {
        changed = ImGui::DragFloat("Fov", &fov_deg, 0.1f, 30.f, 175.f);
        changed |= ImGui::DragFloat("Z Near", &new_znear, 0.001f, 0.01f, 5.0f);
        changed |= ImGui::DragFloat("Z Far", &new_zfar, 1.0f, 2.0f, 3000.0f);
    }

    if (changed && fabsf(new_znear) > 1e-4 && new_znear < new_zfar) {
        if (is_ortho)
            setOrthoParams(ortho_centered, ortho_left, ortho_width, ortho_top, ortho_height, new_znear, new_zfar);
        else
            setProjectionParams(deg2rad(fov_deg), getAspectRatio(), new_znear, new_zfar);
    }

    VEC3 f = getForward();
    ImGui::LabelText("Front", "%f %f %f", f.x, f.y, f.z);
    VEC3 up = getUp();
    ImGui::LabelText("Up", "%f %f %f", up.x, up.y, up.z);
    VEC3 left = getRight();
    ImGui::LabelText("Right", "%f %f %f", left.x, left.y, left.z);

    //ImGui::LabelText("ViewPort", "@(%d,%d) %dx%d", viewport.x0, viewport.y0, viewport.width, viewport.height);
    ImGui::LabelText("Aspect Ratio", "%f", getAspectRatio());
    
    return changed;
}

void TCompCamera::debugInMenu() {
    innerDebugInMenu();
}
