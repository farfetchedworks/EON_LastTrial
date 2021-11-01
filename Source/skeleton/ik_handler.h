#pragma once

#include "mcv_platform.h"

struct TIKHandle {
  float    AB;        // Given from Bone
  float    BC;        // Given from Bone
  VEC3     A, B, C;   // A & C are given, B is found
  float    h;         // Perp amount with respect to AC
  VEC3     normal;

  enum eState {
    NORMAL
  , TOO_FAR
  , TOO_SHORT
  , UNKNOWN
  };
  eState   state = eState::UNKNOWN;

  bool solveB() {

    float AC = (A-C).Length();
    VEC3 dir = C - A;
    dir.Normalize();

    if (AC > AB + BC) {
      state = TOO_FAR;
      B = A + dir * AB;
      return false;
    }

    float num = AB*AB - BC*BC - AC*AC;
    float den = -2 * AC;

    if (den == 0) {
      state = UNKNOWN;
      return false;
    }

    float a2 = num / den;
    float a1 = AC - a2;

    // h^2 + a1^2 = AB^2
    float h2 = AB*AB - a1 * a1;
    if (h2 < 0.f) {
      state = TOO_SHORT;
      B = A - dir * AB;
      return false;
    }
    h = sqrtf(h2);

    VEC3 perp_dir = normal.Cross(dir);
    perp_dir.Normalize();
    B = A + dir * a1 + h * perp_dir;
    state = NORMAL;
    return true;
  }

};

