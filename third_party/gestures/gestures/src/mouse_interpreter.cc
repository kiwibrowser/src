// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gestures/include/mouse_interpreter.h"

#include <math.h>

#include "gestures/include/macros.h"
#include "gestures/include/tracer.h"

namespace gestures {

MouseInterpreter::MouseInterpreter(PropRegistry* prop_reg, Tracer* tracer)
    : Interpreter(NULL, tracer, false),
      wheel_emulation_accu_x_(0.0),
      wheel_emulation_accu_y_(0.0),
      wheel_emulation_active_(false),
      reverse_scrolling_(prop_reg, "Mouse Reverse Scrolling", false),
      scroll_max_allowed_input_speed_(prop_reg,
                                      "Mouse Scroll Max Input Speed",
                                      177.0,
                                      this),
      force_scroll_wheel_emulation_(prop_reg,
                                     "Force Scroll Wheel Emulation",
                                     false),
      scroll_wheel_emulation_speed_(prop_reg,
                                    "Scroll Wheel Emulation Speed",
                                    100.0),
      scroll_wheel_emulation_thresh_(prop_reg,
                                    "Scroll Wheel Emulation Threshold",
                                    1.0) {
  InitName();
  memset(&prev_state_, 0, sizeof(prev_state_));
  memset(&last_wheel_, 0, sizeof(last_wheel_));
  memset(&last_hwheel_, 0, sizeof(last_hwheel_));
  // Scroll acceleration curve coefficients. See the definition for more
  // details on how to generate them.
  scroll_accel_curve_[0] = 1.5937e+01;
  scroll_accel_curve_[1] = 2.5547e-01;
  scroll_accel_curve_[2] = 1.9727e-02;
  scroll_accel_curve_[3] = 1.6313e-04;
  scroll_accel_curve_[4] = -1.0012e-06;
}

void MouseInterpreter::SyncInterpretImpl(HardwareState* hwstate,
                                         stime_t* timeout) {
  if(!EmulateScrollWheel(*hwstate)) {
    // Interpret mouse events in the order of pointer moves, scroll wheels and
    // button clicks.
    InterpretMouseMotionEvent(prev_state_, *hwstate);
    // Note that unlike touchpad scrolls, we interpret and send separate events
    // for horizontal/vertical mouse wheel scrolls. This is partly to match what
    // the xf86-input-evdev driver does and is partly because not all code in
    // Chrome honors MouseWheelEvent that has both X and Y offsets.
    InterpretScrollWheelEvent(*hwstate, true);
    InterpretScrollWheelEvent(*hwstate, false);
    InterpretMouseButtonEvent(prev_state_, *hwstate);
  }
  // Pass max_finger_cnt = 0 to DeepCopy() since we don't care fingers and
  // did not allocate any space for fingers.
  prev_state_.DeepCopy(*hwstate, 0);
}

double MouseInterpreter::ComputeScroll(double input_speed) {
  double result = 0.0;
  double term = 1.0;
  double allowed_speed = fabs(input_speed);
  if (allowed_speed > scroll_max_allowed_input_speed_.val_)
    allowed_speed = scroll_max_allowed_input_speed_.val_;

  // Compute the accelerated scroll value.
  for (size_t i = 0; i < arraysize(scroll_accel_curve_); i++) {
    result += term * scroll_accel_curve_[i];
    term *= allowed_speed;
  }
  if (input_speed < 0)
    result = -result;
  return result;
}

bool MouseInterpreter::EmulateScrollWheel(const HardwareState& hwstate) {
  if (!force_scroll_wheel_emulation_.val_ && hwprops_->has_wheel)
    return false;

  bool down = hwstate.buttons_down & GESTURES_BUTTON_MIDDLE;
  bool prev_down = prev_state_.buttons_down & GESTURES_BUTTON_MIDDLE;
  bool raising = down && !prev_down;
  bool falling = !down && prev_down;

  // Reset scroll emulation detection on button down.
  if (raising) {
    wheel_emulation_accu_x_ = 0;
    wheel_emulation_accu_y_ = 0;
    wheel_emulation_active_ = false;
  }

  // Send button event if button has been released without scrolling.
  if (falling && !wheel_emulation_active_) {
    ProduceGesture(Gesture(kGestureButtonsChange,
                           prev_state_.timestamp,
                           hwstate.timestamp,
                           GESTURES_BUTTON_MIDDLE,
                           GESTURES_BUTTON_MIDDLE));
  }

  if (down) {
    // Detect scroll emulation
    if (!wheel_emulation_active_) {
      wheel_emulation_accu_x_ += hwstate.rel_x;
      wheel_emulation_accu_y_ += hwstate.rel_y;
      double dist_sq = wheel_emulation_accu_x_ * wheel_emulation_accu_x_ +
                       wheel_emulation_accu_y_ * wheel_emulation_accu_y_;
      double thresh_sq = scroll_wheel_emulation_thresh_.val_ *
                         scroll_wheel_emulation_thresh_.val_;
      if (dist_sq > thresh_sq) {
        // Lock into scroll emulation until button is released.
        wheel_emulation_active_ = true;
      }
    }

    // Transform motion into scrolling.
    if (wheel_emulation_active_) {
      double scroll_x = hwstate.rel_x * scroll_wheel_emulation_speed_.val_;
      double scroll_y = hwstate.rel_y * scroll_wheel_emulation_speed_.val_;
      ProduceGesture(Gesture(kGestureScroll, hwstate.timestamp,
                             hwstate.timestamp, scroll_x, scroll_y));
    }
    return true;
  }

  return false;
}

void MouseInterpreter::InterpretScrollWheelEvent(const HardwareState& hwstate,
                                                 bool is_vertical) {
  // Vertical wheel or horizontal wheel.
  float current_wheel_value = hwstate.rel_hwheel;
  WheelRecord* last_wheel_record = &last_hwheel_;
  if (is_vertical) {
    current_wheel_value = hwstate.rel_wheel;
    last_wheel_record = &last_wheel_;
  }

  // Check if the wheel is scrolled.
  if (current_wheel_value) {
    stime_t start_time, end_time = hwstate.timestamp;
    // Check if this scroll is in same direction as previous scroll event.
    if ((current_wheel_value < 0 && last_wheel_record->value < 0) ||
        (current_wheel_value > 0 && last_wheel_record->value > 0)) {
      start_time = last_wheel_record->timestamp;
    } else {
      start_time = end_time;
    }

    // If start_time == end_time, compute click_speed using dt = 1 second.
    stime_t dt = (end_time - start_time) ?: 1.0;
    float offset = ComputeScroll(current_wheel_value / dt);
    last_wheel_record->timestamp = hwstate.timestamp;
    last_wheel_record->value = current_wheel_value;

    if (is_vertical) {
      // For historical reasons the vertical wheel (REL_WHEEL) is inverted
      if (!reverse_scrolling_.val_) {
        offset = -offset;
      }
      ProduceGesture(Gesture(kGestureScroll, start_time, end_time, 0, offset));
    } else {
      ProduceGesture(Gesture(kGestureScroll, start_time, end_time, offset, 0));
    }
  }
}

void MouseInterpreter::InterpretMouseButtonEvent(
    const HardwareState& prev_state, const HardwareState& hwstate) {
  const unsigned buttons[] = {
    GESTURES_BUTTON_LEFT,
    GESTURES_BUTTON_MIDDLE,
    GESTURES_BUTTON_RIGHT,
    GESTURES_BUTTON_BACK,
    GESTURES_BUTTON_FORWARD
  };
  unsigned down = 0, up = 0;

  for (unsigned i = 0; i < arraysize(buttons); i++) {
    if (!(prev_state.buttons_down & buttons[i]) &&
        (hwstate.buttons_down & buttons[i]))
      down |= buttons[i];
    if ((prev_state.buttons_down & buttons[i]) &&
        !(hwstate.buttons_down & buttons[i]))
      up |= buttons[i];
  }

  if (down || up) {
    ProduceGesture(Gesture(kGestureButtonsChange,
                           prev_state.timestamp,
                           hwstate.timestamp,
                           down,
                           up));
  }
}

void MouseInterpreter::InterpretMouseMotionEvent(
    const HardwareState& prev_state,
    const HardwareState& hwstate) {
  if (hwstate.rel_x || hwstate.rel_y) {
    ProduceGesture(Gesture(kGestureMove,
                           prev_state.timestamp,
                           hwstate.timestamp,
                           hwstate.rel_x,
                           hwstate.rel_y));
  }
}

}  // namespace gestures
