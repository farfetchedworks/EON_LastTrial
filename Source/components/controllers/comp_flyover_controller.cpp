#include "mcv_platform.h"
#include "components/common/comp_base.h"
#include "components/common/comp_transform.h"
#include "entity/entity.h"
#include "engine.h"
#include "input/input_module.h"

struct TCompFlyoverController : public TCompBase {

  DECL_SIBLING_ACCESS();

  float speed_factor = 1.0f;
  float rotation_sensibility = 0.5f;
  float movement_reduction_factor = 0.9f;
  VEC3  ispeed = VEC3::Zero;      // inertial speed
  bool  is_enabled = true;
  int   key_toggle_enable = 0;

  void load(const json& j, TEntityParseContext& ctx) {
    //speed_factor =
    speed_factor = j.value("speed_factor", speed_factor);
    is_enabled = j.value("enabled", is_enabled);
    rotation_sensibility = j.value("rotation_sensibility", rotation_sensibility);
    key_toggle_enable = j.value("key_toggle_enable", key_toggle_enable);
    movement_reduction_factor = j.value("movement_reduction_factor", movement_reduction_factor);
  }

  void debugInMenu() {
    ImGui::DragFloat("Max Speed", &speed_factor, 0.1f, 1.f, 100.f);
    ImGui::DragFloat("Sensibility", &rotation_sensibility, 0.001f, 0.001f, 0.1f);
    ImGui::DragFloat("Inertia", &movement_reduction_factor, 0.001f, 0.05f, 1.f);
    ImGui::Checkbox("Enabled", &is_enabled);
  }

  void update(float delta_unused) {
    auto input = CEngine::get().getInput(input::PLAYER_1);

    if (input->getKey((input::EKeyboardKey)key_toggle_enable).getsPressed())
      is_enabled = !is_enabled;

    if (!is_enabled)
      return;

    TCompTransform* c_transform = get<TCompTransform>();
    if (!c_transform)
      return;

    // if the game is paused, we still want full camera speed 
    float dt = Time.delta_unscaled;
    float deltaSpeed = dt * speed_factor;

    VEC3 pos = c_transform->getPosition();
    VEC3 forward = c_transform->getForward();
    VEC3 right = c_transform->getRight();
    VEC3 up = VEC3::Up;

    float yaw, pitch;
    vectorToYawPitch(forward, &yaw, &pitch);

    if (isPressed('W'))
      ispeed.z = 1.f;
    if (isPressed('S'))
      ispeed.z = -1.f;

    if (isPressed('A'))
      ispeed.x = -1.f;
    if (isPressed('D'))
      ispeed.x = 1.f;

    float amount_rotated = deg2rad(rotation_sensibility) * dt;

    if (ImGui::GetIO().MouseDown[1]) {
      ImVec2 mdelta = ImGui::GetIO().MouseDelta;
      yaw -= mdelta.x * amount_rotated;
      pitch += mdelta.y * amount_rotated;
    }

    float max_pitch = deg2rad(89.95f);
    if (pitch > max_pitch)
      pitch = max_pitch;
    else if (pitch < -max_pitch)
      pitch = -max_pitch;

    // Amount in each direction
    VEC3 off;
    off += forward * ispeed.z * deltaSpeed;
    off += right * ispeed.x * deltaSpeed;
    off += up * ispeed.y * deltaSpeed;

    VEC3 new_forward = yawPitchToVector(yaw, pitch);
    VEC3 new_pos = pos + off;
    c_transform->lookAt(new_pos, new_pos + new_forward, up);

    // x50 to have movement_reduction_factor normalized to 0..1
    ispeed *= exp( -movement_reduction_factor * 30.0f * Time.delta_unscaled);
  }

};

DECL_OBJ_MANAGER("flyover_controller", TCompFlyoverController)