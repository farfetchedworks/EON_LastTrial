//****************************************************************************//
// animation_action.cpp                                                       //
// Copyright (C) 2001, 2002 Bruno 'Beosil' Heidelberger                       //
//****************************************************************************//
// This library is free software; you can redistribute it and/or modify it    //
// under the terms of the GNU Lesser General Public License as published by   //
// the Free Software Foundation; either version 2.1 of the License, or (at    //
// your option) any later version.                                            //
//****************************************************************************//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

//****************************************************************************//
// Includes                                                                   //
//****************************************************************************//

#include "cal3d/animation_action.h"
#include "cal3d/error.h"
#include "cal3d/coreanimation.h"

void dbg(const char* format, ...);
#define M_PI       3.14159265358979323846   // pi
#define rad2deg(_rad)    ((_rad) * 180.0f / (float)M_PI)
#define deg2rad(_deg)    ((_deg) / 180.0f * (float)M_PI)

 /*****************************************************************************/
/** Constructs the animation action instance.
  *
  * This function is the default constructor of the animation action instance.
  *****************************************************************************/

CalAnimationAction::CalAnimationAction(CalCoreAnimation* pCoreAnimation)
: CalAnimation(pCoreAnimation)
{
  setType(TYPE_ACTION);
}


 /*****************************************************************************/
/** Executes the animation action instance.
  *
  * This function executes the animation action instance.
  *
  * @param delayIn The time in seconds until the animation action instance
  *                reaches the full weight from the beginning of its execution.
  * @param delayOut The time in seconds in which the animation action instance
  *                 reaches zero weight at the end of its execution.
  * @param weightTarget No doxygen comment for this. FIXME.
  * @param autoLock     This prevents the Action from being reset and removed
  *                     on the last keyframe if true.
  *
  * @return One of the following values:
  *         \li \b true if successful
  *         \li \b false if an error happend
  *****************************************************************************/

bool CalAnimationAction::execute(float delayIn, float delayOut, float weightTarget, bool autoLock, bool extract_root_motion, bool extract_root_yaw, bool play_backwards, float start_time)
{
  setWeight(0.0f);
  m_delayIn = delayIn;
  m_delayOut = delayOut;
  m_weightTarget = weightTarget;
  m_autoLock = autoLock;

  m_total_duration = getCoreAnimation()->getDuration();

  setTime(play_backwards ? m_total_duration : start_time);
  setState(STATE_IN);

  // Added by MCV
  m_extract_root_motion = extract_root_motion;
  m_extract_root_yaw = extract_root_yaw;
  last_position_is_valid = false;
  last_front_is_valid = false;

  return true;
}

float CalAnimationAction::extractRootYaw(float weight, CalQuaternion& rotation)
{
    if (!m_extract_root_yaw)
        return 0.f;

    // new front
    CalMatrix m = CalMatrix(rotation);
    CalVector front = CalVector(m.dxdx, 0, m.dzdx);

    if (last_front_is_valid)
    {
        float dx = front.x * last_front.x + front.z * last_front.z;
        float dy = front.x * last_front.z - front.z * last_front.x;
        root_yaw_delta = std::atan2f(dy, dx);
        last_front = front;
    }
    else
    {
        root_yaw_delta = 0.f;
        last_front_is_valid = true;
        last_front = front;
    }

    // Don't scale by weight so we can get the full rotation
    // root_yaw_delta *= weight;
    acc_root_yaw_delta += root_yaw_delta;

    CalQuaternion q;
    q.x = 0.f;
    q.y = sinf(acc_root_yaw_delta * 0.5f);
    q.z = 0.f;
    q.w = cosf(acc_root_yaw_delta * 0.5f);
    rotation = q * rotation;

    // dbg("acc: %f, yaw: %f, front: x-%f z-%f\n", rad2deg(acc_root_yaw_delta), rad2deg(root_yaw_delta), front.x, front.z);

    return root_yaw_delta;
}

CalVector CalAnimationAction::extractRootMotion(float weight, CalVector& translation)
{
  if( !m_extract_root_motion ) 
    return CalVector(0,0,0);

  if (last_position_is_valid)
  {
    root_motion_delta = translation - last_position;
    last_position = translation;
  }
  else
  {
    root_motion_delta = CalVector(0, 0, 0);
    last_position_is_valid = true;
    last_position = translation;
  }

  // The root bone does NOT move along y
  root_motion_delta.y = 0.0f;
  root_motion_delta *= weight;
  translation.x = 0;
  translation.z = 0;

  return root_motion_delta;
}



 /*****************************************************************************/
/** Updates the animation action instance.
  *
  * This function updates the animation action instance for a given amount of
  * time.
  *
  * @param deltaTime The elapsed time in seconds since the last update.
  *
  * @return One of the following values:
  *         \li \b true if the animation action instance is still active
  *         \li \b false if the execution of the animation action instance has
  *             ended
  *****************************************************************************/

bool CalAnimationAction::update(float deltaTime, bool play_backwards)
{
    // update animation action time

  if(getState() != STATE_STOPPED)
  {
    float time_change = play_backwards ? -deltaTime : deltaTime;
    setTime(getTime() + time_change * getTimeFactor());
  }

  // handle IN phase
  if(getState() == STATE_IN)
  {
    // check if we are still in the IN phase
    if(!play_backwards && getTime() < m_delayIn)
    {
      setWeight(getTime() / m_delayIn * m_weightTarget);
      //m_weight = m_time / m_delayIn;
    }
    else if (play_backwards && m_total_duration - getTime() < m_delayIn)
    {
        setWeight((m_total_duration - getTime()) / m_delayIn * m_weightTarget);
    }
    else
    {
      setState(STATE_STEADY);
      setWeight(m_weightTarget);
    }
  }

  // handle STEADY
  if(getState() == STATE_STEADY)
  {
    // check if we reached OUT phase
    if (!m_autoLock && ((!play_backwards && getTime() >= m_total_duration - m_delayOut) || (play_backwards && getTime() <= m_delayOut)))
    {
      setState(STATE_OUT);
    }
    // if the anim is supposed to stay locked on last keyframe, reset the time here.
    else if (m_autoLock && getTime() > m_total_duration)
    {
      setState(STATE_STOPPED);
      setTime(m_total_duration);
      return false;
    }      
  }

  // handle OUT phase
  if(getState() == STATE_OUT)
  {
    // check if we are still in the OUT phase
    if (!play_backwards && getTime() < m_total_duration)
    {
      setWeight((m_total_duration - getTime()) / m_delayOut * m_weightTarget);
    }
    else if (play_backwards && getTime() >= 0.0f)
    {
      setWeight((getTime()) / m_delayOut * m_weightTarget);
    }
    else
    {
      // we reached the end of the action animation
      setWeight(0.0f);
      return false;
    }
  }

  return true;

}

// ----------------------------------------------------
// MCV
bool CalAnimationAction::remove(float timeout) {
  if (timeout == 0.f)
    return true;

  setState(STATE_OUT);

  // Do we have enough timeout time?
  if (!m_autoLock && getTime() + timeout > m_total_duration)
    timeout = m_total_duration - getTime();

  // Modify official duration
  if (m_autoLock)
    m_total_duration += timeout;
  else 
    m_total_duration = getTime() + timeout;

  // Update timeout
  m_delayOut = timeout;

  // To avoid discontinuities if cancelled during IN/OUT stages
  m_weightTarget = getWeight();

  // Returns false to indicate the animation should NOT be removed
  // from the list of active animations.
  return false;
}


//****************************************************************************//
