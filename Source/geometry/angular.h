#pragma once

#define rad2deg(_rad)    ((_rad) * 180.0f / (float)M_PI)
#define deg2rad(_deg)    ((_deg) / 180.0f * (float)M_PI)

VEC3  yawToVector(float yaw);
float vectorToYaw(VEC3 front);
VEC3  yawPitchToVector(float yaw, float pitch);
void  vectorToYawPitch(VEC3 front, float* yaw, float* pitch);
