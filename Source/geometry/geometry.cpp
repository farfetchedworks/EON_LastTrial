#include "mcv_platform.h"

VEC2 loadVEC2(const std::string& str) {
  VEC2 v;
  int n = sscanf(str.c_str(), "%f %f", &v.x, &v.y);
  if (n == 2) {
    return v;
  }
  fatal("Invalid str reading VEC2 %s. Only %d values read. Expected 2", str.c_str(), n);

  return VEC2::Zero;
}

VEC2 loadVEC2(const json& j, const char* attr) {

  assert(j.is_object());
  if (j.count(attr)) {
    const std::string& str = j.value(attr, "");
    return loadVEC2(str);
  }

  return VEC2::Zero;
}

VEC2 loadVEC2(const json& j, const char* attr, const VEC2& defaultValue) {
  if (j.count(attr) == 0) {
    return defaultValue;
  }

  VEC2 v = defaultValue;
  auto k = j.value(attr, "0 0");
  sscanf(k.c_str(), "%f %f", &v.x, &v.y);
  return v;
}

VEC3 loadVEC3(const std::string& str) {
  VEC3 v;
  int n = sscanf(str.c_str(), "%f %f %f", &v.x, &v.y, &v.z);
  if (n == 3) {
    return v;
  }
  fatal("Invalid str reading VEC3 %s. Only %d values read. Expected 3", str.c_str(), n);

  return VEC3::Zero;
}

VEC3 loadVEC3(const json& j, const char* attr, const VEC3& defaultValue) {

    assert(j.is_object());
    if (j.count(attr)) {
        const std::string& str = j.value(attr, "");
        return loadVEC3(str);
    }

    return defaultValue;
}

VEC4 loadVEC4(const std::string& str) {
  VEC4 v;
  int n = sscanf(str.c_str(), "%f %f %f %f", &v.x, &v.y, &v.z, &v.w);
  if (n == 4) {
    return v;
  }
  fatal("Invalid str reading VEC4 %s. Only %d values read. Expected 4", str.c_str(), n);

  return VEC4::Zero;
}

VEC4 loadVEC4(const json& j, const char* attr, const VEC4& defaultValue) {
  assert(j.is_object());
  if (j.count(attr)) {
    const std::string& str = j.value(attr, "");
    return loadVEC4(str);
  }

  return defaultValue;
}

Color loadColor(const json& j) {
  Color v;
  const auto& str = j.get<std::string>();
  int n = sscanf(str.c_str(), "%f %f %f %f", &v.x, &v.y, &v.z, &v.w);
  if (n == 4)
    return v;
  fatal("Invalid str reading Color %s. Only %d values read. Expected 4\n", str.c_str(), n);
  return Colors::White;
}

Color loadColor(const json& j, const char* attr) {
  assert(j.is_object());
  if (j.count(attr))
    return loadColor( j[attr] );
  return Colors::White;
}

Color loadColor(const json& j, const char* attr, const Color& defaultValue) {
  if (j.count(attr) <= 0)
    return defaultValue;
  return loadColor(j, attr);
}

float clampf(float n, float lower, float upper) {
    return std::max(lower, std::min(n, upper));
}

int clamp(int n, int lower, int upper) {
    return std::max(lower, std::min(n, upper));
}

// Lerps from angle a to b (both between 0.f and PI_TIMES_TWO), taking the shortest path
float lerpRadians(float a, float b, float lerp_velocity, float lerpFactor)
{
    float result;
    float diff = b - a;
    float pi2 = 2.0f * static_cast<float>(M_PI);
    if (diff < -M_PI)
    {
        // lerp upwards past PI_TIMES_TWO
        b += pi2;
        result = damp(a, b, lerp_velocity, lerpFactor);
        if (result >= pi2)
        {
            result -= pi2;
        }
    }
    else if (diff > M_PI)
    {
        // lerp downwards past 0
        b -= pi2;
        result = damp(a, b, lerp_velocity, lerpFactor);
        if (result < 0.f)
        {
            result += pi2;
        }
    }
    else
    {
        // straight lerp
        result = damp(a, b, lerp_velocity, lerpFactor);
    }

    return result;
}

VEC3 normVEC3(VEC3 v) {
    v.Normalize();
    return v;
}

float mapToRange(float in_min, float in_max, float out_min, float out_max, float value) {
    return out_min + ((out_max - out_min) / (in_max - in_min)) * (value - in_min);
}

QUAT getQUATfromMatrix(const MAT44& mat)
{
    float t;
    QUAT q;
    if (mat._33 < 0) {
        if (mat._11 > mat._22) {
            t = 1 + mat._11 - mat._22 - mat._33;
            q = QUAT(t, mat._12 + mat._21, mat._31 + mat._13, mat._23 - mat._32);
        }
        else {
            t = 1 - mat._11 + mat._22 - mat._33;
            q = QUAT(mat._12 + mat._21, t, mat._23 + mat._32, mat._31 - mat._13);
        }
    }
    else {
        if (mat._11 < -mat._22) {
            t = 1 - mat._11 - mat._22 + mat._33;
            q = QUAT(mat._31 + mat._13, mat._23 + mat._32, t, mat._12 - mat._21);
        }
        else {
            t = 1 + mat._11 + mat._22 + mat._33;
            q = QUAT(mat._23 - mat._32, mat._31 - mat._13, mat._12 - mat._21, t);
        }
    }
    q *= 0.5f / sqrt(t);
    return q;
}

QUAT loadQUAT(const json& j, const char* attr) {
  assert(j.is_object());
  if (j.count(attr)) {
    const std::string& str = j.value(attr, "");
    QUAT q;
    int n = sscanf(str.c_str(), "%f %f %f %f", &q.x, &q.y, &q.z, &q.w);
    if (n == 4) {
      return q;
    }
    fatal("Invalid json reading QUAT attr %s. Only %d values read. Expected 4", attr, n);
  }

  return QUAT::Identity;
}

std::vector<int> loadVECN(const json& j, const char* attr)
{
    assert(j.is_object());

    std::vector<int> vecn;

    if (j.contains(attr)) {
        const std::string& str = j.value(attr, "");

        std::stringstream iss(str);
        int f;

        while (iss >> f) {
            vecn.push_back(f);
        }
    }

    return vecn;
}

QUAT dampQUAT(QUAT a, QUAT b, float lambda, float dt)
{
    return QUAT::Lerp(a, b, 1 - exp(-lambda * dt));
}
