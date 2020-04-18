// Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>  // For FRIEND_TEST

#include "gestures/include/gestures.h"
#include "gestures/include/immediate_interpreter.h"
#include "gestures/include/interpreter.h"
#include "gestures/include/mouse_interpreter.h"
#include "gestures/include/prop_registry.h"
#include "gestures/include/tracer.h"

#ifndef GESTURES_MULTITOUCH_MOUSE_INTERPRETER_H_
#define GESTURES_MULTITOUCH_MOUSE_INTERPRETER_H_

namespace gestures {

class Origin {
  // Origin keeps track of the origins of certin events.
 public:
  void PushGesture(const Gesture& result);

  // Return the last time when the buttons go up
  stime_t ButtonGoingUp(int button) const;

 private:
  stime_t button_going_up_left_{0.0};
  stime_t button_going_up_middle_{0.0};
  stime_t button_going_up_right_{0.0};
};

class MultitouchMouseInterpreter : public MouseInterpreter {
  FRIEND_TEST(MultitouchMouseInterpreterTest, SimpleTest);
 public:
  MultitouchMouseInterpreter(PropRegistry* prop_reg, Tracer* tracer);
  virtual ~MultitouchMouseInterpreter() {}

 protected:
  virtual void SyncInterpretImpl(HardwareState* hwstate, stime_t* timeout);
  virtual void Initialize(const HardwareProperties* hw_props,
                          Metrics* metrics, MetricsProperties* mprops,
                          GestureConsumer* consumer);
  virtual void ProduceGesture(const Gesture& gesture);

 private:
  void InterpretMultitouchEvent();

  // We keep this for finger tracking:
  HardwareStateBuffer state_buffer_;
  // We keep this for standard mouse tracking:
  HardwareState prev_state_;
  ScrollEventBuffer scroll_buffer_;

  FingerMap prev_gs_fingers_;
  FingerMap gs_fingers_;

  GestureType prev_gesture_type_;
  GestureType current_gesture_type_;

  // Set to true when scrolls happen. Set to false when a fling happens,
  // or when mouse starts moving.
  bool should_fling_;

  ScrollManager scroll_manager_;
  Gesture prev_result_;
  Origin origin_;

  // This keeps track of where fingers started. Usually this is their original
  // position, but if the mouse is moved, we reset the positions at that time.
  map<short, Vector2, kMaxFingers> start_position_;

  // These fingers have started moving and should cause gestures.
  set<short, kMaxFingers> moving_;

  // Depth of recent scroll event buffer used to compute click.
  IntProperty click_buffer_depth_;
  // Maximum distance for a click
  DoubleProperty click_max_distance_;

  // Lead time of a button going up versus a finger lifting off
  DoubleProperty click_left_button_going_up_lead_time_;
  DoubleProperty click_right_button_going_up_lead_time_;

  // Distance [mm] a finger must deviate from the start position to be
  // considered moving.
  DoubleProperty min_finger_move_distance_;
  // If there is relative motion at or above this magnitude [mm], start
  // positions are reset.
  DoubleProperty moving_min_rel_amount_;
};

}  // namespace gestures

#endif  // GESTURES_MULTITOUCH_MOUSE_INTERPRETER_H_
