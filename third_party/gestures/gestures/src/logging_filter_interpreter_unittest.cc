// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include <gtest/gtest.h>

#include "gestures/include/activity_replay.h"
#include "gestures/include/gestures.h"
#include "gestures/include/interpreter.h"
#include "gestures/include/logging_filter_interpreter.h"
#include "gestures/include/prop_registry.h"
#include "gestures/include/unittest_util.h"
using std::string;

namespace gestures {

class LoggingFilterInterpreterTest : public ::testing::Test {};

class LoggingFilterInterpreterResetLogTestInterpreter : public Interpreter {
 public:
  LoggingFilterInterpreterResetLogTestInterpreter()
      : Interpreter(NULL, NULL, false) {}
 protected:
  virtual void SyncInterpretImpl(HardwareState* hwstate,
                                     stime_t* timeout) {}
  virtual void SetHardwarePropertiesImpl(const HardwareProperties& hw_props) {
  }
  virtual void HandleTimerImpl(stime_t now, stime_t* timeout) {}
};

TEST(LoggingFilterInterpreterTest, LogResetHandlerTest) {
  PropRegistry prop_reg;
  LoggingFilterInterpreterResetLogTestInterpreter* base_interpreter =
      new LoggingFilterInterpreterResetLogTestInterpreter();
  LoggingFilterInterpreter interpreter(&prop_reg, base_interpreter, NULL);

  HardwareProperties hwprops = {
    0, 0, 100, 100,  // left, top, right, bottom
    10,  // x res (pixels/mm)
    10,  // y res (pixels/mm)
    133, 133,  // scrn DPI X, Y
    -1,  // orientation minimum
    2,   // orientation maximum
    2, 5,  // max fingers, max_touch,
    1, 0, 0, 0  //t5r2, semi, button pad
  };

  TestInterpreterWrapper wrapper(&interpreter, &hwprops);
  FingerState finger_state = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID
    0, 0, 0, 0, 10, 0, 50, 50, 1, 0
  };
  HardwareState hardware_state = {
    // time, buttons, finger count, touch count, finger states pointer
    200000, 0, 1, 1, &finger_state, 0, 0, 0, 0
  };
  stime_t timeout = -1.0;
  wrapper.SyncInterpret(&hardware_state, &timeout);
  EXPECT_EQ(interpreter.log_->size(), 1);

  wrapper.SyncInterpret(&hardware_state, &timeout);
  EXPECT_EQ(interpreter.log_->size(), 2);

  // Assume the ResetLog property is set.
  interpreter.logging_reset_.HandleGesturesPropWritten();
  EXPECT_EQ(interpreter.log_->size(), 0);

  wrapper.SyncInterpret(&hardware_state, &timeout);
  EXPECT_EQ(interpreter.log_->size(), 1);
}
}  // namespace gestures
