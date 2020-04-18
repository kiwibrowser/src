// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gestures/include/accel_filter_interpreter.h"

#include <algorithm>
#include <math.h>

#include "gestures/include/gestures.h"
#include "gestures/include/interpreter.h"
#include "gestures/include/logging.h"
#include "gestures/include/macros.h"
#include "gestures/include/tracer.h"

namespace gestures {

// Takes ownership of |next|:
AccelFilterInterpreter::AccelFilterInterpreter(PropRegistry* prop_reg,
                                               Interpreter* next,
                                               Tracer* tracer)
    : FilterInterpreter(NULL, next, tracer, false),
      // Hack: cast tp_custom_point_/mouse_custom_point_/tp_custom_scroll_
      // to float arrays.
      tp_custom_point_prop_(prop_reg, "Pointer Accel Curve",
                            reinterpret_cast<double*>(&tp_custom_point_),
                            sizeof(tp_custom_point_) / sizeof(double)),
      tp_custom_scroll_prop_(prop_reg, "Scroll Accel Curve",
                             reinterpret_cast<double*>(&tp_custom_scroll_),
                             sizeof(tp_custom_scroll_) / sizeof(double)),
      mouse_custom_point_prop_(prop_reg, "Mouse Pointer Accel Curve",
                               reinterpret_cast<double*>(&mouse_custom_point_),
                               sizeof(mouse_custom_point_) / sizeof(double)),
      use_custom_tp_point_curve_(
          prop_reg, "Use Custom Touchpad Pointer Accel Curve", 0),
      use_custom_tp_scroll_curve_(
          prop_reg, "Use Custom Touchpad Scroll Accel Curve", 0),
      use_custom_mouse_curve_(
          prop_reg, "Use Custom Mouse Pointer Accel Curve", 0),
      pointer_sensitivity_(prop_reg, "Pointer Sensitivity", 3),
      scroll_sensitivity_(prop_reg, "Scroll Sensitivity", 3),
      point_x_out_scale_(prop_reg, "Point X Out Scale", 1.0),
      point_y_out_scale_(prop_reg, "Point Y Out Scale", 1.0),
      scroll_x_out_scale_(prop_reg, "Scroll X Out Scale", 3.0),
      scroll_y_out_scale_(prop_reg, "Scroll Y Out Scale", 3.0),
      use_mouse_point_curves_(prop_reg, "Mouse Accel Curves", 0),
      use_mouse_scroll_curves_(prop_reg, "Mouse Scroll Curves", 0),
      use_old_mouse_point_curves_(prop_reg, "Old Mouse Accel Curves", 0),
      min_reasonable_dt_(prop_reg, "Accel Min dt", 0.003),
      max_reasonable_dt_(prop_reg, "Accel Max dt", 0.050),
      last_reasonable_dt_(0.05),
      smooth_accel_(prop_reg, "Smooth Accel", 0),
      last_end_time_(0.0),
      last_mags_size_(0) {
  InitName();
  // Set up default curves.

  // Our pointing curves are the following.
  // x = input speed of movement (mm/s, always >= 0), y = output speed (mm/s)
  // 1: y = x (No acceleration)
  // 2: y = 32x/60   (x < 32), x^2/60   (x < 150), linear with same slope after
  // 3: y = 32x/37.5 (x < 32), x^2/37.5 (x < 150), linear with same slope after
  // 4: y = 32x/30   (x < 32), x^2/30   (x < 150), linear with same slope after
  // 5: y = 32x/25   (x < 32), x^2/25   (x < 150), linear with same slope after

  const float point_divisors[] = { 0.0, // unused
                                   60.0, 37.5, 30.0, 25.0 };  // used


  // i starts as 1 b/c we skip the first slot, since the default is fine for it.
  for (size_t i = 1; i < kMaxAccelCurves; ++i) {
    const float divisor = point_divisors[i];
    const float linear_until_x = 32.0;
    const float init_slope = linear_until_x / divisor;
    point_curves_[i][0] = CurveSegment(linear_until_x, 0, init_slope, 0);
    const float x_border = 150;
    point_curves_[i][1] = CurveSegment(x_border, 1 / divisor, 0, 0);
    const float slope = x_border * 2 / divisor;
    const float y_at_border = x_border * x_border / divisor;
    const float icept = y_at_border - slope * x_border;
    point_curves_[i][2] = CurveSegment(INFINITY, 0, slope, icept);
  }

  const float old_mouse_speed_straight_cutoff[] = { 5.0, 5.0, 5.0, 8.0, 8.0 };
  const float old_mouse_speed_accel[] = { 1.0, 1.4, 1.8, 2.0, 2.2 };

  for (size_t i = 0; i < kMaxAccelCurves; ++i) {
    const float kParabolaA = 1.3;
    const float kParabolaB = 0.2;
    const float cutoff_x = old_mouse_speed_straight_cutoff[i];
    const float cutoff_y =
        kParabolaA * cutoff_x * cutoff_x + kParabolaB * cutoff_x;
    const float line_m = 2.0 * kParabolaA * cutoff_x + kParabolaB;
    const float line_b = cutoff_y - cutoff_x * line_m;
    const float kOutMult = old_mouse_speed_accel[i];

    old_mouse_point_curves_[i][0] =
        CurveSegment(cutoff_x * 25.4, kParabolaA * kOutMult / 25.4,
                     kParabolaB * kOutMult, 0.0);
    old_mouse_point_curves_[i][1] = CurveSegment(INFINITY, 0.0, line_m * kOutMult,
                                             line_b * kOutMult * 25.4);
  }

  // These values were determined empirically through user studies:
  const float kMouseMultiplierA = 0.0311;
  const float kMouseMultiplierB = 3.26;
  const float kMouseCutoff = 195.0;
  const float kMultipliers[] = { 1.2, 1.4, 1.6, 1.8, 2.0 };
  for (size_t i = 0; i < kMaxAccelCurves; ++i) {
    float mouse_a = kMouseMultiplierA * kMultipliers[i] * kMultipliers[i];
    float mouse_b = kMouseMultiplierB * kMultipliers[i];
    float cutoff = kMouseCutoff / kMultipliers[i];
    float second_slope =
        (2.0 * kMouseMultiplierA * kMouseCutoff + kMouseMultiplierB) *
        kMultipliers[i];
    mouse_point_curves_[i][0] = CurveSegment(cutoff, mouse_a, mouse_b, 0.0);
    mouse_point_curves_[i][1] = CurveSegment(INFINITY, 0.0, second_slope, -1182);
  }

  const float scroll_divisors[] = { 0.0, // unused
                                    150, 75.0, 70.0, 65.0 };  // used
  // Our scrolling curves are the following.
  // x = input speed of movement (mm/s, always >= 0), y = output speed (mm/s)
  // 1: y = x (No acceleration)
  // 2: y = 75x/150   (x < 75), x^2/150   (x < 600), linear (initial slope).
  // 3: y = 75x/75    (x < 75), x^2/75    (x < 600), linear (initial slope).
  // 4: y = 75x/70    (x < 75), x^2/70    (x < 600), linear (initial slope).
  // 5: y = 75x/65    (x < 75), x^2/65    (x < 600), linear (initial slope).
  // i starts as 1 b/c we skip the first slot, since the default is fine for it.
  for (size_t i = 1; i < kMaxAccelCurves; ++i) {
    const float divisor = scroll_divisors[i];
    const float linear_until_x = 75.0;
    const float init_slope = linear_until_x / divisor;
    scroll_curves_[i][0] = CurveSegment(linear_until_x, 0, init_slope, 0);
    const float x_border = 600;
    scroll_curves_[i][1] = CurveSegment(x_border, 1 / divisor, 0, 0);
    // For scrolling / flinging we level off the speed.
    const float slope = init_slope;
    const float y_at_border = x_border * x_border / divisor;
    const float icept = y_at_border - slope * x_border;
    scroll_curves_[i][2] = CurveSegment(INFINITY, 0, slope, icept);
  }
}

void AccelFilterInterpreter::ConsumeGesture(const Gesture& gs) {
  Gesture copy = gs;
  CurveSegment* segs = NULL;
  float* dx = NULL;
  float* dy = NULL;

  // Calculate dt and see if it's reasonable
  float dt = copy.end_time - copy.start_time;
  if (dt < min_reasonable_dt_.val_ || dt > max_reasonable_dt_.val_)
    dt = last_reasonable_dt_;
  else
    last_reasonable_dt_ = dt;

  size_t max_segs = kMaxCurveSegs;
  float x_scale = 1.0;
  float y_scale = 1.0;
  float mag = 0.0;
  // The quantities to scale:
  float* scale_out_x = NULL;
  float* scale_out_y = NULL;
  // We scale ordinal values of scroll/fling gestures as well because we use
  // them in Chrome for history navigation (back/forward page gesture) and
  // we will easily run out of the touchpad space if we just use raw values
  // as they are. To estimate the length one needs to scroll on the touchpad
  // to trigger the history navigation:
  //
  // Pixel:
  // 1280 (screen width in DIPs) * 0.25 (overscroll threshold) /
  // (133 / 25.4) (conversion factor from DIP to mm) = 61.1 mm
  // Most other low-res devices:
  // 1366 * 0.25 / (133 / 25.4) = 65.2 mm
  //
  // With current scroll output scaling factor (2.5), we can reduce the length
  // required to about one inch on all devices.
  float* scale_out_x_ordinal = NULL;
  float* scale_out_y_ordinal = NULL;

  switch (copy.type) {
    case kGestureTypeMove:
    case kGestureTypeSwipe:
    case kGestureTypeFourFingerSwipe:
      if (copy.type == kGestureTypeMove) {
        scale_out_x = dx = &copy.details.move.dx;
        scale_out_y = dy = &copy.details.move.dy;
      } else if (copy.type == kGestureTypeSwipe) {
        scale_out_x = dx = &copy.details.swipe.dx;
        scale_out_y = dy = &copy.details.swipe.dy;
      } else {
        scale_out_x = dx = &copy.details.four_finger_swipe.dx;
        scale_out_y = dy = &copy.details.four_finger_swipe.dy;
      }
      if (use_mouse_point_curves_.val_ && use_custom_mouse_curve_.val_) {
        segs = mouse_custom_point_;
        max_segs = kMaxCustomCurveSegs;
      } else if (!use_mouse_point_curves_.val_ &&
                 use_custom_tp_point_curve_.val_) {
        segs = tp_custom_point_;
        max_segs = kMaxCustomCurveSegs;
      } else {
        if (use_mouse_point_curves_.val_) {
          if (use_old_mouse_point_curves_.val_)
            segs = old_mouse_point_curves_[pointer_sensitivity_.val_ - 1];
          else
            segs = mouse_point_curves_[pointer_sensitivity_.val_ - 1];
        } else {
          segs = point_curves_[pointer_sensitivity_.val_ - 1];
        }
      }
      x_scale = point_x_out_scale_.val_;
      y_scale = point_y_out_scale_.val_;
      break;
    case kGestureTypeFling:  // fall through
    case kGestureTypeScroll:
      if (copy.type == kGestureTypeFling) {
        float vx = copy.details.fling.vx;
        float vy = copy.details.fling.vy;
        mag = sqrtf(vx * vx + vy * vy);
        scale_out_x = &copy.details.fling.vx;
        scale_out_y = &copy.details.fling.vy;
        scale_out_x_ordinal = &copy.details.fling.ordinal_vx;
        scale_out_y_ordinal = &copy.details.fling.ordinal_vy;
      } else {
        scale_out_x = dx = &copy.details.scroll.dx;
        scale_out_y = dy = &copy.details.scroll.dy;
        scale_out_x_ordinal = &copy.details.scroll.ordinal_dx;
        scale_out_y_ordinal = &copy.details.scroll.ordinal_dy;
      }
      // We bypass mouse scroll events as they have a separate acceleration
      // algorithm implemented in mouse_interpreter.
      if (use_mouse_scroll_curves_.val_) {
        ProduceGesture(gs);
        return;
      }
      if (!use_custom_tp_scroll_curve_.val_) {
        segs = scroll_curves_[scroll_sensitivity_.val_ - 1];
      } else {
        segs = tp_custom_scroll_;
        max_segs = kMaxCustomCurveSegs;
      }
      x_scale = scroll_x_out_scale_.val_;
      y_scale = scroll_y_out_scale_.val_;
      break;
    default:  // Nothing to accelerate
      ProduceGesture(gs);
      return;
  }

  if (dx != NULL && dy != NULL) {
    if (dt < 0.00001) {
      ProduceGesture(gs);
      return;  // Avoid division by 0
    }
    mag = sqrtf(*dx * *dx + *dy * *dy) / dt;
  }

  if (smooth_accel_.val_) {
    if (last_end_time_ == gs.start_time) {
      float new_mag = mag;
      if (last_mags_size_ < arraysize(last_mags_))
          last_mags_[last_mags_size_] = last_mags_[last_mags_size_ - 1];
      for (size_t i = last_mags_size_ - 1; i > 0; i--) {
        new_mag += last_mags_[i];
        last_mags_[i] = last_mags_[i - 1];
      }
      new_mag += last_mags_[0];
      new_mag /= last_mags_size_ + 1;

      last_mags_[0] = mag;
      last_mags_size_ = std::min(arraysize(last_mags_), last_mags_size_ + 1);
      mag = new_mag;
    } else {
      last_mags_size_ = 1;
      last_mags_[0] = mag;
    }
    last_end_time_ = gs.end_time;
  }

  if (mag < 0.00001) {
    if (gs.type == kGestureTypeFling)
      ProduceGesture(gs);  // Filter out zero length gestures
    return;  // Avoid division by 0
  }

  for (size_t i = 0; i < max_segs; ++i) {
    if (mag > segs[i].x_)
      continue;
    float ratio = segs[i].sqr_ * mag + segs[i].mul_ + segs[i].int_ / mag;
    *scale_out_x *= ratio * x_scale;
    *scale_out_y *= ratio * y_scale;
    if (copy.type == kGestureTypeFling ||
        copy.type == kGestureTypeScroll) {
      // We don't accelerate the ordinal values as we do for normal ones
      // because this is how the Chrome needs it.
      *scale_out_x_ordinal *= x_scale;
      *scale_out_y_ordinal *= y_scale;
    }
    ProduceGesture(copy);
    return;
  }
  Err("Overflowed acceleration curve!");
}

}  // namespace gestures
