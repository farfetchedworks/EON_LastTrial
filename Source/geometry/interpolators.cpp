#pragma once

#include "mcv_platform.h"
#include "interpolators.h"

namespace interpolators
{
#define DEFINE_INTERPOLATOR(__UPPERNAME__,__LOWERNAME__) \
  const T##__UPPERNAME__##Interpolator __LOWERNAME__##Interpolator;

#define DEFINE_INTERPOLATORS_IN_OUT(__UPPERNAME__,__LOWERNAME__) \
  DEFINE_INTERPOLATOR(__UPPERNAME__##In,__LOWERNAME__##In) \
  DEFINE_INTERPOLATOR(__UPPERNAME__##Out,__LOWERNAME__##Out) \
  DEFINE_INTERPOLATOR(__UPPERNAME__##InOut,__LOWERNAME__##InOut)

#define DRAW_INTERPOLATOR(__UPPERNAME__,__LOWERNAME__) \
  renderInterpolator(#__UPPERNAME__, __LOWERNAME__##Interpolator);

#define DRAW_INTERPOLATORS_IN_OUT(__UPPERNAME__,__LOWERNAME__) \
  DRAW_INTERPOLATOR(__UPPERNAME__##In,__LOWERNAME__##In) \
  DRAW_INTERPOLATOR(__UPPERNAME__##Out,__LOWERNAME__##Out) \
  DRAW_INTERPOLATOR(__UPPERNAME__##InOut,__LOWERNAME__##InOut)

  // interpolator instances
  DEFINE_INTERPOLATOR(Linear, linear)
  DEFINE_INTERPOLATORS_IN_OUT(Quad, quad)
  DEFINE_INTERPOLATORS_IN_OUT(Cubic, cubic)
  DEFINE_INTERPOLATORS_IN_OUT(Quart, quart)
  DEFINE_INTERPOLATORS_IN_OUT(Quint, quint)
  DEFINE_INTERPOLATORS_IN_OUT(Back, back)
  DEFINE_INTERPOLATORS_IN_OUT(Elastic, elastic)
  DEFINE_INTERPOLATORS_IN_OUT(Bounce, bounce)
  DEFINE_INTERPOLATORS_IN_OUT(Circular, circular)
  DEFINE_INTERPOLATORS_IN_OUT(Expo, expo)
  DEFINE_INTERPOLATORS_IN_OUT(Sine, sine)

  void renderInterpolator(const char* name, const IInterpolator& interpolator)
  {
    const int nsamples = 50;
    float values[nsamples];
    for (int i = 0; i < nsamples; ++i)
    {
      values[i] = interpolator.blend(0.f, 1.f, static_cast<float>(i) / static_cast<float>(nsamples));
    }
    ImGui::PlotLines(name, values, nsamples, 0, 0,
      std::numeric_limits<float>::min(), std::numeric_limits<float>::max(),
      ImVec2(150, 80));
  }

  void renderInterpolators()
  {
    if (ImGui::TreeNode("Interpolators"))
    {
      DRAW_INTERPOLATOR(Linear, linear)
      DRAW_INTERPOLATORS_IN_OUT(Quad, quad)
      DRAW_INTERPOLATORS_IN_OUT(Cubic, cubic)
      DRAW_INTERPOLATORS_IN_OUT(Quart, quart)
      DRAW_INTERPOLATORS_IN_OUT(Quint, quint)
      DRAW_INTERPOLATORS_IN_OUT(Back, back)
      DRAW_INTERPOLATORS_IN_OUT(Elastic, elastic)
      DRAW_INTERPOLATORS_IN_OUT(Bounce, bounce)
      DRAW_INTERPOLATORS_IN_OUT(Circular, circular)
      DRAW_INTERPOLATORS_IN_OUT(Expo, expo)
      DRAW_INTERPOLATORS_IN_OUT(Sine, sine)

      ImGui::TreePop();
    }
  }
};
