// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <math.h>

#include <memory>
#include <gtest/gtest.h>  // for FRIEND_TEST

#include "gestures/include/filter_interpreter.h"
#include "gestures/include/gestures.h"
#include "gestures/include/prop_registry.h"
#include "gestures/include/tracer.h"

#ifndef GESTURES_ACCEL_FILTER_INTERPRETER_H_
#define GESTURES_ACCEL_FILTER_INTERPRETER_H_

namespace gestures {

// This interpreter provides pointer and scroll acceleration based on
// an acceleration curve and the user's sensitivity setting.

class AccelFilterInterpreter : public FilterInterpreter {
  FRIEND_TEST(AccelFilterInterpreterTest, CustomAccelTest);
  FRIEND_TEST(AccelFilterInterpreterTest, SimpleTest);
  FRIEND_TEST(AccelFilterInterpreterTest, TimingTest);
  FRIEND_TEST(AccelFilterInterpreterTest, TinyMoveTest);
 public:
  // Takes ownership of |next|:
  AccelFilterInterpreter(PropRegistry* prop_reg, Interpreter* next,
                         Tracer* tracer);
  virtual ~AccelFilterInterpreter() {}

  virtual void ConsumeGesture(const Gesture& gs);

 private:
  struct CurveSegment {
    CurveSegment() : x_(INFINITY), sqr_(0.0), mul_(1.0), int_(0.0) {}
    CurveSegment(float x, float s, float m, float b)
        : x_(x), sqr_(s), mul_(m), int_(b) {}
    CurveSegment(const CurveSegment& that)
        : x_(that.x_), sqr_(that.sqr_), mul_(that.mul_), int_(that.int_) {}
    // Be careful adding new members: We currently cast arrays of CurveSegment
    // to arrays of float (to expose to the properties system)
    double x_;  // Max X value of segment. User's point will be less than this.
    double sqr_;  // x^2 multiplier
    double mul_;  // Slope of line (x multiplier)
    double int_;  // Intercept of line
  };

  static const size_t kMaxCurveSegs = 3;
  static const size_t kMaxCustomCurveSegs = 20;
  static const size_t kMaxAccelCurves = 5;

  // curves for sensitivity 1..5
  CurveSegment point_curves_[kMaxAccelCurves][kMaxCurveSegs];
  CurveSegment old_mouse_point_curves_[kMaxAccelCurves][kMaxCurveSegs];
  CurveSegment mouse_point_curves_[kMaxAccelCurves][kMaxCurveSegs];
  CurveSegment scroll_curves_[kMaxAccelCurves][kMaxCurveSegs];

  // Custom curves
  CurveSegment tp_custom_point_[kMaxCustomCurveSegs];
  CurveSegment tp_custom_scroll_[kMaxCustomCurveSegs];
  CurveSegment mouse_custom_point_[kMaxCustomCurveSegs];
  // Note: there is no mouse_custom_scroll_ b/c mouse wheel accel is
  // handled in the MouseInterpreter class.

  // These properties expose the custom curves (just above) to the
  // property system.
  DoubleArrayProperty tp_custom_point_prop_;
  DoubleArrayProperty tp_custom_scroll_prop_;
  DoubleArrayProperty mouse_custom_point_prop_;

  // These properties enable use of custom curves (just above).
  BoolProperty use_custom_tp_point_curve_;
  BoolProperty use_custom_tp_scroll_curve_;
  BoolProperty use_custom_mouse_curve_;

  IntProperty pointer_sensitivity_;  // [1..5]
  IntProperty scroll_sensitivity_;  // [1..5]

  DoubleProperty point_x_out_scale_;
  DoubleProperty point_y_out_scale_;
  DoubleProperty scroll_x_out_scale_;
  DoubleProperty scroll_y_out_scale_;
  // These properties are automatically set on mice-like devices:
  BoolProperty use_mouse_point_curves_;  // set on {touch,nontouch} mice
  BoolProperty use_mouse_scroll_curves_;  // set on nontouch mice
  // If use_mouse_point_curves_ is true, this is consulted to see which
  // curves to use:
  BoolProperty use_old_mouse_point_curves_;

  // Sometimes on wireless hardware (e.g. Bluetooth), patckets need to be
  // resent. This can lead to a time between packets that very large followed
  // by a very small one. Very small periods especially cause problems b/c they
  // make the velocity seem very fast, which leads to an exaggeration of
  // movement.
  // To compensate, we have bounds on what we expect a reasonable period to be.
  // Events that have too large or small a period get reassigned the last
  // reasonable period.
  DoubleProperty min_reasonable_dt_;
  DoubleProperty max_reasonable_dt_;
  stime_t last_reasonable_dt_;

  // If we enable smooth accel, the past few magnitudes are used to compute the
  // multiplication factor.
  BoolProperty smooth_accel_;
  stime_t last_end_time_;
  float last_mags_[2];
  size_t last_mags_size_;
};

}  // namespace gestures

#endif  // GESTURES_SCALING_FILTER_INTERPRETER_H_
