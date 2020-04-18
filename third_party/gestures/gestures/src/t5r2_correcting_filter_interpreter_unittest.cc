// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <deque>
#include <math.h>
#include <vector>
#include <utility>

#include <gtest/gtest.h>

#include "gestures/include/gestures.h"
#include "gestures/include/t5r2_correcting_filter_interpreter.h"
#include "gestures/include/unittest_util.h"
#include "gestures/include/util.h"

using std::deque;
using std::make_pair;
using std::pair;
using std::vector;

namespace gestures {

class T5R2CorrectingFilterInterpreterTest : public ::testing::Test {};

class T5R2CorrectingFilterInterpreterTestInterpreter : public Interpreter {
 public:
  T5R2CorrectingFilterInterpreterTestInterpreter()
      : Interpreter(NULL, NULL, false) {}

  virtual void SyncInterpret(HardwareState* hwstate, stime_t* timeout) {
    if (expected_hardware_state_) {
      EXPECT_EQ(expected_hardware_state_->timestamp, hwstate->timestamp);
      EXPECT_EQ(expected_hardware_state_->buttons_down, hwstate->buttons_down);
      EXPECT_EQ(expected_hardware_state_->finger_cnt, hwstate->finger_cnt);
      EXPECT_EQ(expected_hardware_state_->touch_cnt, hwstate->touch_cnt);
      EXPECT_EQ(expected_hardware_state_->fingers, hwstate->fingers);
    }
    if (return_values_.empty())
      return;
    return_value_ = return_values_.front();
    return_values_.pop_front();
    if (return_value_.type == kGestureTypeNull)
      return;
    ProduceGesture(return_value_);
  }

  virtual void HandleTimer(stime_t now, stime_t* timeout) {
    EXPECT_TRUE(false);
  }

  Gesture return_value_;
  deque<Gesture> return_values_;
  HardwareState* expected_hardware_state_;
};

struct HardwareStateAndExpectations {
  HardwareState hs;
  bool modified;
};

// This test sends a bunch of HardwareStates into the T5R2 correcting
// interpreter and makes sure that, when expected, it alters the hardware
// state.

TEST(T5R2CorrectingFilterInterpreterTest, SimpleTest) {
  T5R2CorrectingFilterInterpreterTestInterpreter* base_interpreter = NULL;
  std::unique_ptr<T5R2CorrectingFilterInterpreter> interpreter;

  HardwareProperties hwprops = {
    0, 0, 10, 10,  // left, top, right, bottom
    1,  // x res (pixels/mm)
    1,  // y res (pixels/mm)
    133, 133,  // scrn DPI X, Y
    -1,  // orientation minimum
    2,   // orientation maximum
    2, 5,  // max fingers, max_touch
    0, 0, 0, 0  //t5r2, semi, button pad
  };
  TestInterpreterWrapper wrapper(interpreter.get(), &hwprops);

  FingerState fs[] = {
    { 0, 0, 0, 0, 1, 0, 150, 4000, 1, 0 },
    { 0, 0, 0, 0, 2, 0, 550, 2000, 2, 0 }
  };
  HardwareStateAndExpectations hse[] = {
    // normal case -- no change expected
    { { 0.01, 0, 1, 1, &fs[0], 0, 0, 0, 0 }, false },
    { { 0.02, 0, 1, 3, &fs[0], 0, 0, 0, 0 }, false },
    { { 0.03, 0, 2, 3, &fs[0], 0, 0, 0, 0 }, false },
    { { 0.04, 0, 0, 0, NULL, 0, 0, 0, 0   }, false },
    // problem -- change expected at end
    { { 0.01, 0, 2, 3, &fs[0], 0, 0, 0, 0 }, false },
    { { 0.02, 0, 2, 3, &fs[0], 0, 0, 0, 0 }, false },
    { { 0.03, 0, 0, 1, NULL, 0, 0, 0, 0   }, false },
    { { 0.04, 0, 0, 1, NULL, 0, 0, 0, 0   }, true },
    // problem -- change expected at end
    { { 0.01, 0, 1, 1, &fs[0], 0, 0, 0, 0 }, false },
    { { 0.02, 0, 1, 3, &fs[0], 0, 0, 0, 0 }, false },
    { { 0.03, 0, 2, 3, &fs[0], 0, 0, 0, 0 }, false },
    { { 0.04, 0, 0, 2, NULL, 0, 0, 0, 0   }, false },
    { { 0.05, 0, 0, 2, NULL, 0, 0, 0, 0   }, true }
  };

  for (size_t i = 0; i < arraysize(hse); i++) {
    // Reset the interpreter for each of the cases
    if (hse[i].hs.timestamp == 0.01) {
      base_interpreter =
          new T5R2CorrectingFilterInterpreterTestInterpreter;
      interpreter.reset(
          new T5R2CorrectingFilterInterpreter(NULL, base_interpreter, NULL));
      wrapper.Reset(interpreter.get());
    }
    HardwareState expected_hs = hse[i].hs;
    if (hse[i].modified)
      expected_hs.touch_cnt = 0;
    base_interpreter->expected_hardware_state_ = &expected_hs;
    stime_t timeout = -1.0;
    EXPECT_EQ(NULL, wrapper.SyncInterpret(&hse[i].hs, &timeout));
    base_interpreter->expected_hardware_state_ = NULL;
    EXPECT_LT(timeout, 0.0);
  }
}

}  // namespace gestures
