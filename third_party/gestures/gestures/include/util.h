// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GESTURES_UTIL_H_
#define GESTURES_UTIL_H_

#include <math.h>

#include "gestures/include/gestures.h"
#include "gestures/include/interpreter.h"

namespace gestures {

inline bool FloatEq(float a, float b) {
  return fabsf(a - b) <= 1e-5;
}

inline bool DoubleEq(float a, float b) {
  return fabsf(a - b) <= 1e-8;
}

// Returns the square of the distance between the two contacts.
template<typename ContactTypeA, typename ContactTypeB>
float DistSq(const ContactTypeA& finger_a, const ContactTypeB& finger_b) {
  float dx = finger_a.position_x - finger_b.position_x;
  float dy = finger_a.position_y - finger_b.position_y;
  return dx * dx + dy * dy;
}
template<typename ContactType>  // UnmergedContact or FingerState
float DistSqXY(const ContactType& finger_a, float pos_x, float pos_y) {
  float dx = finger_a.position_x - pos_x;
  float dy = finger_a.position_y - pos_y;
  return dx * dx + dy * dy;
}

template<typename ContactType>
int CompareX(const void* a_ptr, const void* b_ptr) {
  const ContactType* a = *static_cast<const ContactType* const*>(a_ptr);
  const ContactType* b = *static_cast<const ContactType* const*>(b_ptr);
  return a->position_x < b->position_x ? -1 : a->position_x > b->position_x;
}

template<typename ContactType>
int CompareY(const void* a_ptr, const void* b_ptr) {
  const ContactType* a = *static_cast<const ContactType* const*>(a_ptr);
  const ContactType* b = *static_cast<const ContactType* const*>(b_ptr);
  return a->position_y < b->position_y ? -1 : a->position_y > b->position_y;
}

inline float DegToRad(float degrees) {
  return M_PI * degrees / 180.0;
}

// Combines two gesture objects, storing the result into *gesture.
// Priority is given to button events first, then *gesture.
// Also, |gesture| comes earlier in time than |addend|.
void CombineGestures(Gesture* gesture, const Gesture* addend);

}  // namespace gestures

#endif  // GESTURES_UTIL_H_
