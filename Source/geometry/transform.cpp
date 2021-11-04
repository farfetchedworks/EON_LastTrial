#include "mcv_platform.h"
#include "transform.h"
#include "imgui/ImGuizmo.h"
#include "render/draw_primitives.h"

MAT44 CTransform::asMatrix() const {
  return MAT44::CreateScale(scale) 
    * MAT44::CreateFromQuaternion(rotation) 
    * MAT44::CreateTranslation(position);
}

void CTransform::fromMatrix(MAT44 mtx) {
    mtx.Decompose(scale, rotation, position);
}

void CTransform::fromMatrix(CTransform t) {
    t.asMatrix().Decompose(scale, rotation, position);
}

VEC3 CTransform::getForward() const {
    return normVEC3(-asMatrix().Forward());
}

VEC3 CTransform::getUp() const {
    return normVEC3(asMatrix().Up());;
}

VEC3 CTransform::getRight() const {
    return normVEC3(-asMatrix().Right());
}

void CTransform::lookAt(VEC3 src, VEC3 target, VEC3 up_aux) {
  position = src;
  VEC3 front = target - src;
  float yaw, pitch;
  vectorToYawPitch(front, &yaw, &pitch);
  setEulerAngles(yaw, pitch, 0.f);
}

void CTransform::setEulerAngles(float yaw, float pitch, float roll) {
  rotation = QUAT::CreateFromYawPitchRoll(yaw, pitch, roll);
}

void CTransform::getEulerAngles(float* yaw, float* pitch, float* roll) const {
  VEC3 forward = getForward();
  vectorToYawPitch(forward, yaw, pitch);

  // If requested...
  if (roll) {
    VEC3 roll_zero_left = VEC3(0, 1, 0).Cross(forward);
    VEC3 roll_zero_up = VEC3(forward).Cross(roll_zero_left);
    VEC3 my_real_left = -getRight();
    my_real_left.Normalize();
    roll_zero_left.Normalize();
    roll_zero_up.Normalize();
    float rolled_left_on_up = my_real_left.Dot(roll_zero_up);
    float rolled_left_on_left = my_real_left.Dot(roll_zero_left);
    *roll = atan2f(rolled_left_on_up, rolled_left_on_left);
  }
}


CTransform CTransform::combinedWith(const CTransform& delta_transform) const {
  CTransform new_transform;
  new_transform.rotation = delta_transform.rotation * rotation;

  VEC3 delta_pos_rotated = VEC3::Transform(delta_transform.position, rotation);
  new_transform.position = position + (delta_pos_rotated * scale);

  new_transform.scale = scale * delta_transform.scale;
  return new_transform;
}

bool CTransform::renderInMenu() {
  bool changed = false;
  changed |= ImGui::DragFloat3("Position", &position.x, 0.01f, -100.0f, 100.0f);
  //float yaw_deg = rad2deg(yaw);
  //if (ImGui::DragFloat("Yaw", &yaw_deg, 1.0f, -180.0f, 180.0f))
  //  yaw = deg2rad(yaw_deg);
  changed |= ImGui::DragFloat3("Scale", &scale.x, 0.01f, 0.0f, 5.0f);
  changed |= ImGui::DragFloat4("Rotation", &rotation.x, 0.01f, -5.f, 5.0f);
  VEC3 forward = getForward();
  ImGui::LabelText("Forward", "%f %f %f", forward.x, forward.y, forward.z);
  VEC3 right = getRight();
  ImGui::LabelText("Right", "%f %f %f", right.x, right.y, right.z);
  VEC3 up = getUp();
  ImGui::LabelText("Up", "%f %f %f", up.x, up.y, up.z);

  changed |= renderGuizmo();

  return changed;
}

bool CTransform::renderGuizmo() {
  // Get the lower 32 bits of my address
  ImGuizmo::SetID(((std::ptrdiff_t)this) & 0xffffffff);

  static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
  static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);
  if (ImGui::IsKeyPressed(90))
    mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
  if (ImGui::IsKeyPressed(69))
    mCurrentGizmoOperation = ImGuizmo::ROTATE;
  if (ImGui::IsKeyPressed(82)) // r Key
    mCurrentGizmoOperation = ImGuizmo::SCALE;
  if (ImGui::RadioButton("Translate", mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
    mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
  ImGui::SameLine();
  if (ImGui::RadioButton("Rotate", mCurrentGizmoOperation == ImGuizmo::ROTATE))
    mCurrentGizmoOperation = ImGuizmo::ROTATE;
  ImGui::SameLine();
  if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
    mCurrentGizmoOperation = ImGuizmo::SCALE;
  ImGui::SameLine();

  if (mCurrentGizmoOperation != ImGuizmo::SCALE)
  {
    if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
      mCurrentGizmoMode = ImGuizmo::LOCAL;
    ImGui::SameLine();
    if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
      mCurrentGizmoMode = ImGuizmo::WORLD;
  }
  
  bool changed = false;
  ImGui::SameLine();
  if (ImGui::SmallButton("Reset")) {
    if (mCurrentGizmoOperation == ImGuizmo::TRANSLATE)
      setPosition(VEC3(0, 0, 0));
    else if (mCurrentGizmoOperation == ImGuizmo::ROTATE)
      setRotation(QUAT());
    else if (mCurrentGizmoOperation == ImGuizmo::SCALE)
      scale = VEC3(1, 1, 1);
    changed = true;
  }
  ImGui::SameLine();
  if (ImGui::SmallButton("All")) {
    setPosition(VEC3(0, 0, 0));
    setRotation(QUAT());
    scale = VEC3(1, 1, 1);
    changed = true;
  }

  MAT44 mtx = asMatrix();

  // Using full screen
  ImGuiIO& io = ImGui::GetIO();
  ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

  const CCamera& camera = getActiveCamera();

  changed |= ImGuizmo::Manipulate(
    &camera.getView()._11,
    &camera.getProjection()._11,
    mCurrentGizmoOperation,
    mCurrentGizmoMode,
    &mtx._11      // Addr is of the first float of the matrix
  );

  if (changed) {
    // Update myself if the manipulator has changed the transform
    fromMatrix(mtx);
  }

  return changed;
}

bool CTransform::isInFront(VEC3 p) const {
  return getForward().Dot(p - position) > 0.0f;
}

bool CTransform::isInsideCone(VEC3 p, float full_fov_radians, float z_cone) const {
  float angle_to_p = getYawRotationToAimTo(p);
  float dist = VEC3::Distance(p, position);
  return (fabsf(angle_to_p) < (full_fov_radians * 0.5f)) && (dist<z_cone);
}

float CTransform::getYawRotationToAimTo(VEC3 p) const {
  VEC3 dir = p - position;
  float dx = getForward().Dot(dir);
  float dy = -getRight().Dot(dir);
  float angle = std::atan2f(dy, dx);
  return angle;
}

float CTransform::getPitchRotationToAimTo(VEC3 p) const {
    VEC3 dir = p - position;
    float dx = getForward().Dot(dir);
    float dy = -getUp().Dot(dir);
    float angle = std::atan2f(dy, dx);
    return angle;
}

float CTransform::distance(const CTransform& t) const {
    return VEC3::Distance(this->getPosition(), t.getPosition());
}

bool CTransform::fromJson(const json& j) {
  if (j.count("pos")) {
    position = loadVEC3(j, "pos");
  }
  if (j.count("lookat")) {
    lookAt(getPosition(), loadVEC3(j, "lookat"), VEC3(0, 1, 0));
  }
  if (j.count("rot")) {
    rotation = loadQUAT(j, "rot");
  }
  if (j.count("euler")) {
    VEC3 euler = loadVEC3(j, "euler");
    euler.x = deg2rad(euler.x);
    euler.y = deg2rad(euler.y);
    euler.z = deg2rad(euler.z);
    setEulerAngles(euler.x, euler.y, euler.z);
  }
  if (j.count("scale")) {
    const json& jscale = j["scale"];
    if (jscale.is_number()) { 
      float fscale = jscale.get<float>();
      scale = VEC3(fscale);
    }
    else {
      scale = loadVEC3(j, "scale");
    }
  }
  return true;
}

void CTransform::interpolateTo(const CTransform& target, float amount_of_target)
{
  assert(amount_of_target >= 0 && amount_of_target <= 1.f);
  position = VEC3::Lerp(position, target.position, amount_of_target);
  rotation = QUAT::Slerp(rotation, target.rotation, amount_of_target);
  scale = VEC3::Lerp(scale, target.scale, amount_of_target);
}