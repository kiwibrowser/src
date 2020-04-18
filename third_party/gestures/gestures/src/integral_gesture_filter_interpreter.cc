// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gestures/include/integral_gesture_filter_interpreter.h"

#include <math.h>

#include "gestures/include/gestures.h"
#include "gestures/include/interpreter.h"
#include "gestures/include/tracer.h"

namespace gestures {

// Takes ownership of |next|:
IntegralGestureFilterInterpreter::IntegralGestureFilterInterpreter(
    Interpreter* next, Tracer* tracer)
    : FilterInterpreter(NULL, next, tracer, false),
      hscroll_remainder_(0.0),
      vscroll_remainder_(0.0),
      hscroll_ordinal_remainder_(0.0),
      vscroll_ordinal_remainder_(0.0) {
  InitName();
}

void IntegralGestureFilterInterpreter::SyncInterpretImpl(
    HardwareState* hwstate, stime_t* timeout) {
  if (hwstate->finger_cnt == 0 && hwstate->touch_cnt == 0)
    hscroll_ordinal_remainder_ = vscroll_ordinal_remainder_ =
        hscroll_remainder_ = vscroll_remainder_ = 0.0;
  next_->SyncInterpret(hwstate, timeout);
}

namespace {
float Truncate(float input, float* overflow) {
  input += *overflow;
  float input_ret = truncf(input);
  *overflow = input - input_ret;
  return input_ret;
}
}  // namespace {}

// Truncate the fractional part off any input, but store it. If the
// absolute value of an input is < 1, we will change it to 0, unless
// there has been enough fractional accumulation to bring it above 1.
void IntegralGestureFilterInterpreter::ConsumeGesture(const Gesture& gesture) {
  Gesture copy = gesture;
  switch (gesture.type) {
    case kGestureTypeMove:
      if (gesture.details.move.dx != 0.0 || gesture.details.move.dy != 0.0 ||
          gesture.details.move.ordinal_dx != 0.0 ||
          gesture.details.move.ordinal_dy != 0.0)
        ProduceGesture(gesture);
      break;
    case kGestureTypeScroll:
      copy.details.scroll.dx = Truncate(copy.details.scroll.dx,
                                        &hscroll_remainder_);
      copy.details.scroll.dy = Truncate(copy.details.scroll.dy,
                                        &vscroll_remainder_);
      copy.details.scroll.ordinal_dx = Truncate(copy.details.scroll.ordinal_dx,
                                                &hscroll_ordinal_remainder_);
      copy.details.scroll.ordinal_dy = Truncate(copy.details.scroll.ordinal_dy,
                                                &vscroll_ordinal_remainder_);
      if (copy.details.scroll.dx != 0.0 || copy.details.scroll.dy != 0.0 ||
          copy.details.scroll.ordinal_dx != 0.0 ||
          copy.details.scroll.ordinal_dy != 0.0) {
        ProduceGesture(copy);
      } else if (copy.details.scroll.stop_fling) {
        ProduceGesture(Gesture(kGestureFling, copy.start_time, copy.end_time,
                               0, 0, GESTURES_FLING_TAP_DOWN));
      }
      break;
    default:
      ProduceGesture(gesture);
      break;
  }
}

}  // namespace gestures
