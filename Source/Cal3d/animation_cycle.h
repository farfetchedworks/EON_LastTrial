//****************************************************************************//
// animation_cycle.h                                                          //
// Copyright (C) 2001, 2002 Bruno 'Beosil' Heidelberger                       //
//****************************************************************************//
// This library is free software; you can redistribute it and/or modify it    //
// under the terms of the GNU Lesser General Public License as published by   //
// the Free Software Foundation; either version 2.1 of the License, or (at    //
// your option) any later version.                                            //
//****************************************************************************//

#ifndef CAL_ANIMATION_CYCLE_H
#define CAL_ANIMATION_CYCLE_H


#include "cal3d/global.h"
#include "cal3d/animation.h"


class CalCoreAnimation;


class CAL3D_API CalAnimationCycle : public CalAnimation
{
public:
  CalAnimationCycle(CalCoreAnimation* pCoreAnimation);
  virtual ~CalAnimationCycle() { }

  bool blend(float weight, float delay, bool play_backwards = false);
  void setAsync(float time, float duration);
  bool update(float deltaTime);

  void setTargetWeight(float w) { m_targetWeight = w; }
  void setTargetDelay(float t) { m_targetDelay = t; }

  float getTargetWeight() { return m_targetWeight; }

private:
  float m_targetDelay;
  float m_targetWeight;
};

#endif

//****************************************************************************//
