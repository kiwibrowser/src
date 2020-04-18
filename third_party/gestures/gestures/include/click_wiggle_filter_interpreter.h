// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <gtest/gtest.h>  // for FRIEND_TEST

#include "gestures/include/filter_interpreter.h"
#include "gestures/include/finger_metrics.h"
#include "gestures/include/gestures.h"
#include "gestures/include/map.h"
#include "gestures/include/prop_registry.h"
#include "gestures/include/tracer.h"

#ifndef GESTURES_CLICK_WIGGLE_FILTER_INTERPRETER_H_
#define GESTURES_CLICK_WIGGLE_FILTER_INTERPRETER_H_

namespace gestures {

// This interpreter looks for accidental wiggle that occurs near a physical
// button click event and suppresses that motion.

class ClickWiggleFilterInterpreter : public FilterInterpreter {
  FRIEND_TEST(ClickWiggleFilterInterpreterTest, SimpleTest);
  FRIEND_TEST(ClickWiggleFilterInterpreterTest, OneFingerClickSuppressTest);

 public:
  struct ClickWiggleRec {
    float x_, y_;  // position where we started blocking wiggle
    stime_t began_press_suppression_;  // when we started blocking inc/dec press
    bool suppress_:1;
    bool operator==(const ClickWiggleRec& that) const {
      return x_ == that.x_ &&
          y_ == that.y_ &&
          began_press_suppression_ == that.began_press_suppression_ &&
          suppress_ == that.suppress_;
    }
    bool operator!=(const ClickWiggleRec& that) const {
      return !(*this == that);
    }
  };

  // Takes ownership of |next|:
  ClickWiggleFilterInterpreter(PropRegistry* prop_reg, Interpreter* next,
                               Tracer* tracer);
  virtual ~ClickWiggleFilterInterpreter() {}

 protected:
  virtual void SyncInterpretImpl(HardwareState* hwstate, stime_t* timeout);

 private:
  void UpdateClickWiggle(const HardwareState& hwstate);
  void SetWarpFlags(HardwareState* hwstate) const;

  map<short, ClickWiggleRec, kMaxFingers> wiggle_recs_;

  // last time a physical button up or down edge occurred
  stime_t button_edge_occurred_;

  // If there was just one finger on the pad when the button changed
  bool button_edge_with_one_finger_;

  map<short, float, kMaxFingers> prev_pressure_;

  int prev_buttons_;

  // Movements that may be tap/click wiggle larger than this are allowed
  DoubleProperty wiggle_max_dist_;
  // Wiggles (while button up) suppressed only for this time length.
  DoubleProperty wiggle_suppress_timeout_;
  // Wiggles after physical button going down are suppressed for this time
  DoubleProperty wiggle_button_down_timeout_;
  // Time [s] to block single-finger movement after a single finger clicks
  // the physical button down.
  DoubleProperty one_finger_click_wiggle_timeout_;
};

}  // namespace gestures

#endif  // GESTURES_CLICK_WIGGLE_FILTER_INTERPRETER_H_
