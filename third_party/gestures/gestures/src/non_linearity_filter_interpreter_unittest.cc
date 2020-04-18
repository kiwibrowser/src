// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>

#include "gestures/include/gestures.h"
#include "gestures/include/non_linearity_filter_interpreter.h"
#include "gestures/include/unittest_util.h"
#include "gestures/include/util.h"

const char kTestNonlinearData[] =
    "data/non_linearity_data/testing_non_linearity_data.dat";

namespace gestures {

class NonLinearityFilterTest : public ::testing::Test {};

class NonLinearityFilterInterpreterTestInterpreter : public Interpreter {
 public:
  NonLinearityFilterInterpreterTestInterpreter()
      : Interpreter(NULL, NULL, false) {}

  virtual void SyncInterpret(HardwareState* hwstate, stime_t* timeout) {}
};

TEST(NonLinearityFilterInterpreterTest, DisablingTest) {
  FingerState finger_state = { 0, 0, 0, 0, 35, 0, 999, 500, 1, 0 };
  HardwareState hwstate = { 200000, 0, 2, 2, &finger_state, 0, 0, 0, 0 };

  NonLinearityFilterInterpreterTestInterpreter* base =
                            new NonLinearityFilterInterpreterTestInterpreter;
  NonLinearityFilterInterpreter interpreter(NULL, base, NULL);
  TestInterpreterWrapper wrapper(&interpreter);

  // Nothing should change since it is disabled by default without data
  EXPECT_EQ(NULL, wrapper.SyncInterpret(&hwstate, NULL));
  EXPECT_FLOAT_EQ(hwstate.fingers[0].position_x, 999);
  EXPECT_FLOAT_EQ(hwstate.fingers[0].position_y, 500);

  // Nothing should change even though it's "enabled" since there is no data
  interpreter.enabled_.val_ = 1;
  EXPECT_EQ(NULL, wrapper.SyncInterpret(&hwstate, NULL));
  EXPECT_FLOAT_EQ(hwstate.fingers[0].position_x, 999);
  EXPECT_FLOAT_EQ(hwstate.fingers[0].position_y, 500);

  // Even with data loaded, if it is not enabled nothing should change
  interpreter.data_location_.val_ = kTestNonlinearData;
  interpreter.LoadData();
  interpreter.enabled_.val_ = 0;
  EXPECT_EQ(NULL, wrapper.SyncInterpret(&hwstate, NULL));
  EXPECT_FLOAT_EQ(hwstate.fingers[0].position_x, 999);
  EXPECT_FLOAT_EQ(hwstate.fingers[0].position_y, 500);
}

TEST(NonLinearityFilterInterpreterTest, HWstateModificationTest) {
  FingerState finger_state = { 0, 0, 0, 0, 0.2, 0, 0.1, 0.3, 1, 0 };
  HardwareState hwstate = { 200000, 0, 1, 1, &finger_state, 0, 0, 0, 0 };

  NonLinearityFilterInterpreterTestInterpreter* base =
                            new NonLinearityFilterInterpreterTestInterpreter;
  NonLinearityFilterInterpreter interpreter(NULL, base, NULL);
  TestInterpreterWrapper wrapper(&interpreter);
  interpreter.enabled_.val_ = 1;
  interpreter.data_location_.val_ = kTestNonlinearData;
  interpreter.LoadData();

  // This reading should be modified slightly by the testing filter
  // with errors of (0.325000, -0.325000)
  EXPECT_EQ(NULL, wrapper.SyncInterpret(&hwstate, NULL));
  EXPECT_FLOAT_EQ((float)hwstate.fingers[0].position_x, 0.1 - 0.325);
  EXPECT_FLOAT_EQ((float)hwstate.fingers[0].position_y, 0.3 + 0.325);
}

TEST(NonLinearityFilterInterpreterTest, HWstateNoChangesNeededTest) {
  FingerState finger_states[] = {
    { 0, 0, 0, 0, 0.5, 0, 0.5, 0.5, 1, 0 },
    { 0, 0, 0, 0, 0.12, 0, 0.78, 0.34, 2, 0 },
  };

  HardwareState hwstates[] = {
    { 200000, 0, 2, 2, finger_states, 0, 0, 0, 0 },
    { 200100, 0, 1, 1, finger_states, 0, 0, 0, 0 },
  };

  NonLinearityFilterInterpreterTestInterpreter* base =
                            new NonLinearityFilterInterpreterTestInterpreter;
  NonLinearityFilterInterpreter interpreter(NULL, base, NULL);
  TestInterpreterWrapper wrapper(&interpreter);
  interpreter.enabled_.val_ = 1;
  interpreter.data_location_.val_ = kTestNonlinearData;
  interpreter.LoadData();

  // Nothing should change since 2 fingers are on the touchpad
  EXPECT_EQ(NULL, wrapper.SyncInterpret(&hwstates[0], NULL));
  EXPECT_FLOAT_EQ(hwstates[0].fingers[0].position_x, 0.5);
  EXPECT_FLOAT_EQ(hwstates[0].fingers[0].position_y, 0.5);
  EXPECT_FLOAT_EQ(hwstates[0].fingers[1].position_x, 0.78);
  EXPECT_FLOAT_EQ(hwstates[0].fingers[1].position_y, 0.34);

  // This finger is at (0.5, 0.5, 0.5) which has 0 error in the test readings
  EXPECT_EQ(NULL, wrapper.SyncInterpret(&hwstates[1], NULL));
  EXPECT_FLOAT_EQ(hwstates[1].fingers[0].position_x, 0.5);
  EXPECT_FLOAT_EQ(hwstates[1].fingers[0].position_y, 0.5);
}

}  // namespace gestures
