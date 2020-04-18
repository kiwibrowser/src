// Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>

#include "gestures/include/gestures.h"
#include "gestures/include/iir_filter_interpreter.h"
#include "gestures/include/unittest_util.h"

namespace gestures {

class IirFilterInterpreterTest : public ::testing::Test {};

class IirFilterInterpreterTestInterpreter : public Interpreter {
 public:
  IirFilterInterpreterTestInterpreter()
      : Interpreter(NULL, NULL, false),
        sync_interpret_cnt_(0) {
    prev_.position_x = 0.0;
    prev_.position_y = 0.0;
  }

  virtual void SyncInterpret(HardwareState* hwstate, stime_t* timeout) {
    if (sync_interpret_cnt_) {
      EXPECT_GT(hwstate->fingers[0].position_x, prev_.position_x);
      EXPECT_GT(hwstate->fingers[0].position_y, prev_.position_y);
    }
    EXPECT_EQ(1, hwstate->finger_cnt);
    prev_ = hwstate->fingers[0];
    sync_interpret_cnt_++;
  }

  virtual void HandleTimer(stime_t now, stime_t* timeout) {}

  FingerState prev_;
  size_t sync_interpret_cnt_;
};

TEST(IirFilterInterpreterTest, SimpleTest) {
  IirFilterInterpreterTestInterpreter* base_interpreter =
      new IirFilterInterpreterTestInterpreter;
  IirFilterInterpreter interpreter(NULL, base_interpreter, NULL);
  TestInterpreterWrapper wrapper(&interpreter);

  FingerState fs[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID
    { 0, 0, 0, 0, 30, 0, 1, 1, 1, GESTURES_FINGER_WARP_X },
    { 0, 0, 0, 0, 30, 0, 2, 2, 1, 0 },
    { 0, 0, 0, 0, 30, 0, 3, 3, 1, 0 },
    { 0, 0, 0, 0, 30, 0, 5, 5, 1, 0 }
  };
  HardwareState hs[] = {
    { 0.000, 0, 1, 1, &fs[0], 0, 0, 0, 0 },
    { 0.010, 0, 1, 1, &fs[1], 0, 0, 0, 0 },
    { 0.020, 0, 1, 1, &fs[2], 0, 0, 0, 0 },
    { 0.030, 0, 1, 1, &fs[3], 0, 0, 0, 0 }
  };

  for (size_t i = 0; i < arraysize(hs); i++) {
    unsigned expected_flags = hs[i].fingers[0].flags;
    wrapper.SyncInterpret(&hs[i], NULL);
    EXPECT_EQ(base_interpreter->prev_.flags, expected_flags);
  }
  EXPECT_EQ(arraysize(hs), base_interpreter->sync_interpret_cnt_);
}

TEST(IirFilterInterpreterTest, DisableIIRTest) {
  IirFilterInterpreterTestInterpreter* base_interpreter =
      new IirFilterInterpreterTestInterpreter;
  IirFilterInterpreter interpreter(NULL, base_interpreter, NULL);
  TestInterpreterWrapper wrapper(&interpreter);

  FingerState fs[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID
    { 0, 0, 0, 0, 30, 0, 10, 10, 1, 0 },
    { 0, 0, 0, 0, 30, 0, 11, 15, 1, 0 },
    { 0, 0, 0, 0, 30, 0, 12, 30, 1, 0 },
    { 0, 0, 0, 0, 30, 0, 13, 31, 1, 0 },
    { 0, 0, 0, 0, 30, 0, 14, 32, 1, 0 },
    { 0, 0, 0, 0, 30, 0, 14, 32, 1, 0 },
  };
  HardwareState hs[] = {
    { 0.000, 0, 1, 1, &fs[0], 0, 0, 0, 0 },
    { 0.010, 0, 1, 1, &fs[1], 0, 0, 0, 0 },
    { 0.020, 0, 1, 1, &fs[2], 0, 0, 0, 0 },
    { 0.030, 0, 1, 1, &fs[3], 0, 0, 0, 0 },
    { 0.040, 0, 1, 1, &fs[4], 0, 0, 0, 0 },
    { 0.050, 0, 1, 1, &fs[5], 0, 0, 0, 0 },
  };

  for (size_t i = 0; i < arraysize(hs); i++) {
    wrapper.SyncInterpret(&hs[i], NULL);
    // A quick move at hs[2] and IIR will be disabled. Even though
    // hs[2] and hs[3] are close enough, the rolling average output
    // of hs[2] is smoothed that IIR is still disabled for hs[3].
    // After hs[3], the actual output of hs[i] is approaching hs[i] so
    // IIR filter will be re-enabled.
    if (i >= 2 && i <= 3)
      EXPECT_EQ(interpreter.using_iir_, false);
    else
      EXPECT_EQ(interpreter.using_iir_, true);
  }
}

TEST(IirFilterInterpreterTest, SemiMTIIRTest) {
  IirFilterInterpreterTestInterpreter* base_interpreter =
      new IirFilterInterpreterTestInterpreter;
  IirFilterInterpreter interpreter(NULL, base_interpreter, NULL);

  HardwareProperties hwprops = {
    0, 0, 100, 60,  // left, top, right, bottom
    1.0, 1.0, 25.4, 25.4, // x res, y res, x DPI, y DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    2, 3, 0, 0, 0, 0  // max_fingers, max_touch, t5r2, semi_mt,
  };
  TestInterpreterWrapper wrapper(&interpreter, &hwprops);

  float kTestPressure = 100;
  FingerState fs_normal[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID
    { 0, 0, 0, 0, 30, 0, 5, 5, 1, 0 },
    { 0, 0, 0, 0, kTestPressure, 0, 6, 6, 1, 0 },
  };

  HardwareState hs_normal[] = {
    { 0.000, 0, 1, 1, &fs_normal[0], 0, 0, 0, 0 },
    { 0.010, 0, 1, 1, &fs_normal[1], 0, 0, 0, 0 },
  };

  // For Non-SemiMT, the pressure of the finger will be different from the
  // original one after the IIR filter.
  for (size_t i = 0; i < arraysize(hs_normal); i++)
    wrapper.SyncInterpret(&hs_normal[i], NULL);
  int n = arraysize(fs_normal);
  EXPECT_NE(fs_normal[n - 1].pressure, kTestPressure);

  // On the other hand, for SemiMT, the pressure of the finger should remain the
  // same after IIR filter.
  FingerState fs_semi_mt[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID
    { 0, 0, 0, 0, 30, 0, 5, 5, 1, 0 },
    { 0, 0, 0, 0, kTestPressure, 0, 6, 6, 1, 0 },
  };
  HardwareState hs_semi_mt[] = {
    { 0.000, 0, 1, 1, &fs_semi_mt[0], 0, 0, 0, 0 },
    { 0.010, 0, 1, 1, &fs_semi_mt[1], 0, 0, 0, 0 },
  };
  hwprops.support_semi_mt = true;
  wrapper.Reset(&interpreter, &hwprops);
  for (size_t i = 0; i < arraysize(hs_semi_mt); i++)
    wrapper.SyncInterpret(&hs_semi_mt[i], NULL);
  n = arraysize(fs_semi_mt);
  EXPECT_EQ(fs_semi_mt[n - 1].pressure, kTestPressure);
}

}  // namespace gestures
