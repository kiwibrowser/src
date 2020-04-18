// Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <deque>
#include <math.h>
#include <vector>
#include <utility>

#include <gtest/gtest.h>

#include "gestures/include/gestures.h"
#include "gestures/include/integral_gesture_filter_interpreter.h"
#include "gestures/include/unittest_util.h"
#include "gestures/include/util.h"

using std::deque;
using std::make_pair;
using std::pair;

namespace gestures {

class IntegralGestureFilterInterpreterTest : public ::testing::Test {};

class IntegralGestureFilterInterpreterTestInterpreter : public Interpreter {
 public:
  IntegralGestureFilterInterpreterTestInterpreter()
      : Interpreter(NULL, NULL, false) {}

  virtual void SyncInterpret(HardwareState* hwstate, stime_t* timeout) {
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
  deque<std::vector<pair<float, float> > > expected_coordinates_;
};

TEST(IntegralGestureFilterInterpreterTestInterpreter, OverflowTest) {
  IntegralGestureFilterInterpreterTestInterpreter* base_interpreter =
      new IntegralGestureFilterInterpreterTestInterpreter;
  IntegralGestureFilterInterpreter interpreter(base_interpreter, NULL);
  TestInterpreterWrapper wrapper(&interpreter);

  // causing finger, dx, dy, fingers, buttons down, buttons mask, hwstate:
  base_interpreter->return_values_.push_back(
      Gesture(kGestureScroll, 0, 0, -20.9, 4.2));
  base_interpreter->return_values_.push_back(
      Gesture(kGestureScroll, 0, 0, .8, 1.7));
  base_interpreter->return_values_.push_back(
      Gesture(kGestureScroll, 0, 0, -0.8, 2.2));
  base_interpreter->return_values_.push_back(
      Gesture(kGestureScroll, 0, 0, -0.2, 0));
  base_interpreter->return_values_.push_back(
      Gesture(kGestureScroll, 0, 0, -0.2, 0));

  base_interpreter->return_values_[base_interpreter->return_values_.size() -
                                   1].details.scroll.stop_fling = 1;

  FingerState fs = { 0, 0, 0, 0, 1, 0, 0, 0, 1, 0 };
  HardwareState hs = { 10000, 0, 1, 1, &fs, 0, 0, 0, 0 };

  GestureType expected_types[] = {
    kGestureTypeScroll,
    kGestureTypeScroll,
    kGestureTypeScroll,
    kGestureTypeScroll,
    kGestureTypeFling
  };
  float expected_x[] = {
    -20, 0, 0, -1, 0
  };
  float expected_y[] = {
    4, 1, 3, 0, 0
  };

  ASSERT_EQ(arraysize(expected_types), arraysize(expected_x));
  ASSERT_EQ(arraysize(expected_types), arraysize(expected_y));

  for (size_t i = 0; i < arraysize(expected_x); i++) {
    Gesture* out = wrapper.SyncInterpret(&hs, NULL);
    if (out)
      EXPECT_EQ(expected_types[i], out->type) << "i = " << i;
    if (out == NULL) {
      EXPECT_FLOAT_EQ(expected_x[i], 0.0) << "i = " << i;
      EXPECT_FLOAT_EQ(expected_y[i], 0.0) << "i = " << i;
    } else if (out->type == kGestureTypeFling) {
      EXPECT_FLOAT_EQ(GESTURES_FLING_TAP_DOWN, out->details.fling.fling_state)
          << "i = " << i;
    } else {
      EXPECT_FLOAT_EQ(expected_x[i], out->details.scroll.dx) << "i = " << i;
      EXPECT_FLOAT_EQ(expected_y[i], out->details.scroll.dy) << "i = " << i;
    }
  }
}

// This test scrolls by 3.9 pixels, which causes an output of 3px w/ a
// stored remainder of 0.9 px. Then, all fingers are removed, which should
// reset the remainders. Then scroll again by 0.2 pixels, which would
// result in a 1px scroll if the remainders weren't cleared.
TEST(IntegralGestureFilterInterpreterTest, ResetTest) {
  IntegralGestureFilterInterpreterTestInterpreter* base_interpreter =
      new IntegralGestureFilterInterpreterTestInterpreter;
  IntegralGestureFilterInterpreter interpreter(base_interpreter, NULL);
  TestInterpreterWrapper wrapper(&interpreter);

  // causing finger, dx, dy, fingers, buttons down, buttons mask, hwstate:
  base_interpreter->return_values_.push_back(
      Gesture(kGestureScroll, 0, 0, 3.9, 0.0));
  base_interpreter->return_values_.push_back(
      Gesture());
  base_interpreter->return_values_.push_back(
      Gesture(kGestureScroll, 0, 0, .2, 0.0));

  FingerState fs = { 0, 0, 0, 0, 1, 0, 0, 0, 1, 0 };
  HardwareState hs[] = {
    { 10000.00, 0, 1, 1, &fs, 0, 0, 0, 0 },
    { 10000.01, 0, 0, 0, NULL, 0, 0, 0, 0 },
    { 10000.02, 0, 1, 1, &fs, 0, 0, 0, 0 },
  };

  size_t iter = 0;
  Gesture* out = wrapper.SyncInterpret(&hs[iter++], NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), out);
  EXPECT_EQ(kGestureTypeScroll, out->type);
  out = wrapper.SyncInterpret(&hs[iter++], NULL);
  EXPECT_EQ(reinterpret_cast<Gesture*>(NULL), out);
  out = wrapper.SyncInterpret(&hs[iter++], NULL);
  EXPECT_EQ(reinterpret_cast<Gesture*>(NULL), out);
}

// This test requests (0.0, 0.0) move and scroll.
// Both should result in a null gestures.
TEST(IntegralGestureFilterInterpreterTest, ZeroGestureTest) {
  IntegralGestureFilterInterpreterTestInterpreter* base_interpreter =
      new IntegralGestureFilterInterpreterTestInterpreter;
  IntegralGestureFilterInterpreter interpreter(base_interpreter, NULL);
  TestInterpreterWrapper wrapper(&interpreter);

  // causing finger, dx, dy, fingers, buttons down, buttons mask, hwstate:
  base_interpreter->return_values_.push_back(
      Gesture(kGestureMove, 0, 0, 0.0, 0.0));
  base_interpreter->return_values_.push_back(
      Gesture(kGestureScroll, 0, 0, 0.0, 0.0));

  HardwareState hs[] = {
    { 10000.00, 0, 0, 0, NULL, 0, 0, 0, 0 },
    { 10000.01, 0, 0, 0, NULL, 0, 0, 0, 0 },
  };

  size_t iter = 0;
  Gesture* out = wrapper.SyncInterpret(&hs[iter++], NULL);
  EXPECT_EQ(reinterpret_cast<Gesture*>(NULL), out);
  out = wrapper.SyncInterpret(&hs[iter++], NULL);
  EXPECT_EQ(reinterpret_cast<Gesture*>(NULL), out);
}

}  // namespace gestures
