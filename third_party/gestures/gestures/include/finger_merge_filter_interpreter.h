// Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>  // for FRIEND_TEST

#include "gestures/include/filter_interpreter.h"
#include "gestures/include/finger_metrics.h"
#include "gestures/include/gestures.h"
#include "gestures/include/map.h"
#include "gestures/include/prop_registry.h"
#include "gestures/include/set.h"
#include "gestures/include/tracer.h"

#ifndef GESTURES_FINGER_MERGE_FILTER_INTERPRETER_H_
#define GESTURES_FINGER_MERGE_FILTER_INTERPRETER_H_

namespace gestures {

// This interpreter mainly detects finger merging and mark the merging
// finger(s) with a new flag GESTURE_FINGER_MERGE. The flag can be used
// in immediateinterpreter to suppress cursor jumps/moves caused by the
// merging/merged finger(s).

class FingerMergeFilterInterpreter : public FilterInterpreter {

 public:
  FingerMergeFilterInterpreter(PropRegistry* prop_reg, Interpreter* next,
                               Tracer* tracer);
  virtual ~FingerMergeFilterInterpreter() {}

 protected:
  virtual void SyncInterpretImpl(HardwareState* hwstate, stime_t* timeout);

 private:
  // Detects finger merge and appends GESTURE_FINGER_MERGE flag for a merged
  // finger or close fingers
  void UpdateFingerMergeState(const HardwareState& hwstate);

  bool IsSuspiciousAngle(const FingerState& fs) const;

  struct Start {
    float position_x;
    float position_y;
    stime_t start_time;
    bool operator==(const Start& that) const {
      return position_x == that.position_x &&
          position_y == that.position_y &&
          start_time == that.start_time;
    }
    bool operator!=(const Start& that) const {
      return !(*this == that);
    }
  };

  // Info about each contact's initial state
  map<short, Start, kMaxFingers> start_info_;

  set<short, kMaxFingers> merge_tracking_ids_;

  // Fingers that should never merge, as we've determined they aren't a merge
  set<short, kMaxFingers> never_merge_ids_;

  map<short, float, kMaxFingers> prev_x_displacement_;
  map<short, float, kMaxFingers> prev2_x_displacement_;

  // Flag to turn on/off the finger merge filter
  BoolProperty finger_merge_filter_enable_;

  // Distance threshold for close fingers
  DoubleProperty merge_distance_threshold_;

  // Maximum pressure value of a merged finger candidate
  DoubleProperty max_pressure_threshold_;

  // Min pressure of a merged finger candidate
  DoubleProperty min_pressure_threshold_;

  // Minimum touch major of a merged finger candidate
  DoubleProperty min_major_threshold_;

  // More criteria for filtering out false positives:
  // The touch major of a merged finger could be merged_major_pressure_ratio_
  // times of its pressure value at least or a merged finger could have a
  // very high touch major
  DoubleProperty merged_major_pressure_ratio_;
  DoubleProperty merged_major_threshold_;

  // We require that when a finger has displaced in the X direction more than
  // x_jump_min_displacement_, the next frame it must be at
  // x_jump_max_displacement_, else it's not considered a merged finger.
  DoubleProperty x_jump_min_displacement_;
  DoubleProperty x_jump_max_displacement_;

  // If a contact has displaced from the start position more than
  // suspicious_angle_min_displacement_, we require it to be at a particular
  // angle relative to the start position.
  DoubleProperty suspicious_angle_min_displacement_;

  // If a contact exceeds any of these (movement in a direction or age),
  // it is not marked as a merged contact.
  DoubleProperty max_x_move_;
  DoubleProperty max_y_move_;
  DoubleProperty max_age_;
};

}

#endif
