// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>  // For FRIEND_TEST

#include "gestures/include/gestures.h"
#include "gestures/include/interpreter.h"
#include "gestures/include/prop_registry.h"
#include "gestures/include/tracer.h"

#ifndef GESTURES_MOUSE_INTERPRETER_H_
#define GESTURES_MOUSE_INTERPRETER_H_

namespace gestures {

class MouseInterpreter : public Interpreter, public PropertyDelegate {
  FRIEND_TEST(MouseInterpreterTest, SimpleTest);
 public:
  MouseInterpreter(PropRegistry* prop_reg, Tracer* tracer);
  virtual ~MouseInterpreter() {};

 protected:
  virtual void SyncInterpretImpl(HardwareState* hwstate, stime_t* timeout);
  // These functions interpret mouse events, which include button clicking and
  // mouse movement. This function needs two consecutive HardwareState. If no
  // mouse events are presented, result object is not modified. Scroll wheel
  // events are not interpreted as they are handled differently for normal
  // mice and multi-touch mice (ignored for multi-touch mice and accelerated
  // for normal mice).
  void InterpretMouseButtonEvent(const HardwareState& prev_state,
                                 const HardwareState& hwstate);

  void InterpretMouseMotionEvent(const HardwareState& prev_state,
                                 const HardwareState& hwstate);
  // Check for scroll wheel events and produce scroll gestures.
  void InterpretScrollWheelEvent(const HardwareState& hwstate,
                                 bool is_vertical);
  bool EmulateScrollWheel(const HardwareState& hwstate);
 private:
  struct WheelRecord {
    WheelRecord(float v, stime_t t): value(v), timestamp(t) {}
    WheelRecord(): value(0), timestamp(0) {}
    float value;
    stime_t timestamp;
  };

  // Accelerate mouse scroll offsets so that it is larger when the user scroll
  // the mouse wheel faster.
  double ComputeScroll(double input_speed);

  HardwareState prev_state_;

  // Records last scroll wheel event.
  WheelRecord last_wheel_, last_hwheel_;

  // Accumulators to measure scroll distance while doing scroll wheel emulation
  double wheel_emulation_accu_x_;
  double wheel_emulation_accu_y_;

  // True while wheel emulation is locked in.
  bool wheel_emulation_active_;

  // Reverse wheel scrolling.
  BoolProperty reverse_scrolling_;

  // We use normal CDF to simulate scroll wheel acceleration curve. Use the
  // following method to generate the coefficients of a degree-4 polynomial
  // regression for a specific normal cdf in matlab.
  //
  // Note: x for click_speed, y for scroll pixels.
  // In reality, x ranges from 1 to 120+ for an Apple Mighty Mouse, use range
  // greater than that to minimize approximation error at the end points.
  // In our case, the range is [-50, 200].
  //
  // matlab/octave code to generate polynomial coefficients below:
  // x = [-50:200];
  // y = 580 * normcdf(x,100,40) + 20;
  // coeff = polyfit(x,y,4);

  // y_approximated = a0 + a1*x + a2*x^2 + a3*x^3 + a4*x^4
  double scroll_accel_curve_[5];

  // when x is 177, the polynomial curve gives 450, the max pixels to scroll.
  DoubleProperty scroll_max_allowed_input_speed_;

  // Force scroll wheel emulation for any devices
  BoolProperty force_scroll_wheel_emulation_;

  // Multiplication factor to translate cursor motion into scrolling
  DoubleProperty scroll_wheel_emulation_speed_;

  // Movement distance after which to start scroll wheel emulation [in mm]
  DoubleProperty scroll_wheel_emulation_thresh_;
};

}  // namespace gestures

#endif  // GESTURES_MOUSE_INTERPRETER_H_
