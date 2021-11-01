#include "mcv_platform.h"
#include "engine.h"
#include "input/input_module.h"
#include "components/messages.h"
#include "components/common/comp_transform.h"
#include "components/cameras/comp_camera_flyover.h"

DECL_OBJ_MANAGER("camera_flyover", TCompCameraFlyover)

void TCompCameraFlyover::onEntityCreated() {

	input = CEngine::get().getInput(input::PLAYER_1);
	assert(input);
	name = "camera_flyover";

	TMsgCameraCreated msg;
	msg.name = name;
	msg.camera = CHandle(this);
	msg.active = true;
	GameManager->sendMsg(msg);
}

void TCompCameraFlyover::load(const json& j, TEntityParseContext& ctx) {
	speed_factor = j.value("speed_factor", speed_factor);
	rotation_sensibility = j.value("rotation_sensibility", rotation_sensibility);
}

void TCompCameraFlyover::update(float delta_unused) {

	if (!isWndFocused() || !enabled)
		return;

	TCompTransform* c_transform = get<TCompTransform>();
	if (!c_transform) return;

	// if the game is paused, we still want full camera speed 
	float dt = Time.delta_unscaled;
	float deltaSpeed = dt * speed_factor;

	VEC3 pos = c_transform->getPosition();
	VEC3 forward = c_transform->getForward();
	VEC3 right = c_transform->getRight();
	VEC3 up = VEC3::Up;

	float yaw, pitch;
	vectorToYawPitch(forward, &yaw, &pitch);

	// Exit game
	if (isPressed(VK_ESCAPE)) {
		CApplication::get().exit();
	}

	if (isPressed('W')) ispeed.z = 1.f;
	if (isPressed('S')) ispeed.z = -1.f;
	if (isPressed('A')) ispeed.x = -1.f;
	if (isPressed('D')) ispeed.x = 1.f;

	if(isPressed(VK_SHIFT)) deltaSpeed *= 5.f;
	if(isPressed(VK_MENU)) deltaSpeed *= 0.2f;

	float amount_rotated = deg2rad(rotation_sensibility) * dt;

	if (ImGui::GetIO().MouseDown[1]) {
		ImVec2 mdelta = ImGui::GetIO().MouseDelta;
		yaw -= mdelta.x * amount_rotated;
		pitch += mdelta.y * amount_rotated;
	}

	float max_pitch = deg2rad(89.95f);
	if (pitch > max_pitch)			pitch = max_pitch;
	else if (pitch < -max_pitch)	pitch = -max_pitch;

	// Amount in each direction
	VEC3 off;
	off += forward * ispeed.z * deltaSpeed;
	off += right * ispeed.x * deltaSpeed;
	off += up * ispeed.y * deltaSpeed;

	VEC3 new_forward = yawPitchToVector(yaw, pitch);
	VEC3 new_pos = pos + off;
	c_transform->lookAt(new_pos, new_pos + new_forward, up);

	ispeed *= 0.90f;
}