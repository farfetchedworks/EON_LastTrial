//****************************************************************************//
// animation_action.h                                                         //
// Copyright (C) 2001, 2002 Bruno 'Beosil' Heidelberger                       //
//****************************************************************************//
// This library is free software; you can redistribute it and/or modify it    //
// under the terms of the GNU Lesser General Public License as published by   //
// the Free Software Foundation; either version 2.1 of the License, or (at    //
// your option) any later version.                                            //
//****************************************************************************//

#ifndef CAL_ANIMATION_ACTION_H
#define CAL_ANIMATION_ACTION_H


#include "cal3d/global.h"
#include "cal3d/animation.h"
#include "cal3d/vector.h"


class CalCoreAnimation;


class CAL3D_API CalAnimationAction : public CalAnimation
{
public:
  CalAnimationAction(CalCoreAnimation* pCoreAnimation);
  virtual ~CalAnimationAction() { }

  bool execute(float delayIn, float delayOut, float weightTarget = 1.0f,bool autoLock=false,bool extract_root_motion=false, bool extract_root_yaw = false, bool play_backwards = false, float start_time = 0.0f);
  bool update(float deltaTime, bool play_backwards = false);

  // Added by MCV
  bool remove(float timeout);
  CalVector extractRootMotion(float weight, CalVector& translation);
  float extractRootYaw(float weight, CalQuaternion& rotation);

private:
  float m_delayIn;
  float m_delayOut;
  float m_delayTarget;
  float m_weightTarget;
  bool  m_autoLock; 
  
  // Added by MCV
  float         m_total_duration;

  // Root motion from a single animation action
  bool          m_extract_root_motion = false;
  bool          last_position_is_valid = false;
  CalVector     last_position;
  CalVector     root_motion_delta;

  // Get yaw delta
  bool			m_extract_root_yaw = false;
  bool          last_front_is_valid = false;
  CalVector		last_front;
  float			root_yaw_delta;
  float			acc_root_yaw_delta;
};

#endif
