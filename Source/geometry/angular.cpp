#include "mcv_platform.h"

VEC3  yawToVector(float yaw) {
  // si yaw = 0, devolver 0,0,1
  return VEC3(sinf(yaw), 0.0f, cosf(yaw));
}

float vectorToYaw(VEC3 front) {
  return atan2f(front.x, front.z);
}

VEC3  yawPitchToVector(float yaw, float pitch) {
  return VEC3(
    sinf(yaw) * cosf(-pitch),
                sinf(-pitch),
    cosf(yaw) * cosf(-pitch)
  );
}

void  vectorToYawPitch(VEC3 front, float* yaw, float* pitch) {
  *yaw = atan2f(front.x, front.z);
  float mdo = sqrtf(front.x * front.x + front.z * front.z);
  *pitch = atan2f(-front.y, mdo);
}