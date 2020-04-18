// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>  // For FRIEND_TEST

#include "gestures/include/filter_interpreter.h"
#include "gestures/include/finger_metrics.h"
#include "gestures/include/gestures.h"
#include "gestures/include/map.h"
#include "gestures/include/prop_registry.h"
#include "gestures/include/tracer.h"

#ifndef GESTURES_SENSOR_JUMP_FILTER_INTERPRETER_H_
#define GESTURES_SENSOR_JUMP_FILTER_INTERPRETER_H_

namespace gestures {

// This filter interpreter looks for fingers that suspiciously jump position,
// something that can occur on some touchpads when a finger is large and
// spanning multiple sensors.

// When a suspicious jump is detected, the finger is flagged with a warp
// flag for the axis in which the jump occurred.

class SensorJumpFilterInterpreter : public FilterInterpreter,
                                    public PropertyDelegate {
  FRIEND_TEST(SensorJumpFilterInterpreterTest, SimpleTest);
  FRIEND_TEST(SensorJumpFilterInterpreterTest, ActualLogTest);
 public:
  // Takes ownership of |next|:
  SensorJumpFilterInterpreter(PropRegistry* prop_reg, Interpreter* next,
                              Tracer* tracer);
  virtual ~SensorJumpFilterInterpreter() {}

 protected:
  virtual void SyncInterpretImpl(HardwareState* hwstate, stime_t* timeout);

 private:
  // Whether or not this filter is enabled. If disabled, it behaves as a
  // simple passthrough.
  BoolProperty enabled_;
  // The smallest distance that may be considered for warp for non-cursor
  // movement.
  DoubleProperty min_warp_dist_non_move_;
  // The largest distance that may be considered for warp for non-cursor
  // movement.
  DoubleProperty max_warp_dist_non_move_;
  // A suspiciously sized jump may be tolerated if it's length falls within
  // similar_multiplier_ * the previous frame's movement, for non-cursor
  // movement.
  DoubleProperty similar_multiplier_non_move_;

  // Similar set of parameters to the above ones but for cursor movement.
  DoubleProperty min_warp_dist_move_;
  DoubleProperty max_warp_dist_move_;
  DoubleProperty similar_multiplier_move_;

  // Considering the following X position sequence:
  // 8,  9, 10,  9, 10, 11, 12  (1F move)
  // In case of 1F move, we don't want to marke the second 10 as WARP due
  // to the direction change in  10->9->10 since in Box Filter the second
  // 10 will be re-set to box center and the following 11, 12 won't cause
  // movement since they are within the new box. So we use
  // no_warp_min_dist_move_ to filter out really small direction change to
  // not be marked as WARP.
  DoubleProperty no_warp_min_dist_move_;

  // Fingers from the previous two SyncInterpret calls. previous_input_[0]
  // is the more recent.
  map<short, FingerState, kMaxFingers> previous_input_[2];

  // When a finger is flagged with a warp flag for the first time, we note it
  // here.
  // first_flag_[0] for WARP_X_NON_MOVE;
  // first_flag_[1] for WARP_Y_NON_MOVE;
  // first_flag_[2] for WARP_X_MOVE;
  // first_flag_[3] for WARP_Y_MOVE.
  set<short, kMaxFingers> first_flag_[4];
};

}  // namespace gestures

#endif  // GESTURES_SENSOR_JUMP_FILTER_INTERPRETER_H_
