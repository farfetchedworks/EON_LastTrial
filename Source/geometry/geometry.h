#pragma once

#include "SimpleMath.h"

using MAT44 = DirectX::SimpleMath::Matrix;
using QUAT = DirectX::SimpleMath::Quaternion;
using VEC2 = DirectX::SimpleMath::Vector2;
using VEC3 = DirectX::SimpleMath::Vector3;
using VEC4 = DirectX::SimpleMath::Vector4;
using Color = VEC4;
using AABB = DirectX::BoundingBox;

VEC2 loadVEC2(const std::string& str);
VEC2 loadVEC2(const json& j, const char* attr, const VEC2& defaultValue = VEC2::Zero);
VEC3 loadVEC3(const std::string& str);
VEC3 loadVEC3(const json& j, const char* attr, const VEC3& defaultValue = VEC3::Zero);
VEC4 loadVEC4(const std::string& str);
VEC4 loadVEC4(const json& j, const char* attr, const VEC4& defaultValue = VEC4::Zero);
QUAT loadQUAT(const json& j, const char* attr);
Color loadColor(const json& j);
Color loadColor(const json& j, const char* attr);
Color loadColor(const json& j, const char* attr, const Color& defaultValue);
std::vector<int> loadVECN(const json& j, const char* attr);

VEC3 normVEC3(VEC3 v);

float clampf(float n, float lower, float upper);
int clamp(int n, int lower, int upper);

template<typename T>
T lerp(T a, T b, float v) {
    v = clampf(v, 0.0f, 1.0f);
    return (T)(a * (1.0f - v) + b * v);
}

template<typename T>
T cubicIn(T start, T end, float v) {
    v = clampf(v, 0.0f, 1.0f);
    const T c = end - start;
    return c * v * v * v + start;
}

// pos_player_t = pos_orig * (1.0 - v) + pos_dest * v;
// pos_orig = -(pos_dest * v - pos_player_t) / (1.0 - v);
template<typename T>
T projectLerp(T b, T midVal, float v) {
    return -(b * v - midVal) / (1.0f - v);
}

template<typename T>
T damp(T a, T b, float lambda, float dt)
{
    return lerp(a, b, 1 - exp(-lambda * dt));
}

float smoothDamp(float current, float target, float& currentVelocity, float smoothTime, float deltaTime, float maxSpeed = FLT_MAX);

// From Unity: https://github.com/Unity-Technologies/UnityCsReference/blob/master/Runtime/Export/Math/Vector3.cs#L97
VEC3 smoothDamp(VEC3 current, VEC3 target, VEC3& currentVelocity, float smoothTime, float deltaTime, float maxSpeed = FLT_MAX);

template<typename T>
T dampCubicIn(T a, T b, float lambda, float dt) {
    return cubicIn(a, b, 1 - exp(-lambda * dt));
}

template <typename T>
bool sameSign(typename T x, typename T y)
{
    return (x >= 0) ^ (y < 0);
}

QUAT dampQUAT(QUAT a, QUAT b, float lambda, float dt);

// from https://gist.github.com/itsmrpeck/be41d72e9d4c72d2236de687f6f53974
float lerpRadians(float a, float b, float lerp_velocity, float lerpFactor);

// Maps value in [in_min, in_max] range to [out_min, out_max] range
float mapToRange(float in_min, float in_max, float out_min, float out_max, float value);

QUAT getQUATfromMatrix(const MAT44& mat);

#include "angular.h"
#include "camera.h"
#include "transform.h"
#include "interpolators.h"

