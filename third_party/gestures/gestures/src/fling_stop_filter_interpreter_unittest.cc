// Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>

#include "gestures/include/fling_stop_filter_interpreter.h"
#include "gestures/include/gestures.h"
#include "gestures/include/unittest_util.h"
#include "gestures/include/util.h"

namespace gestures {

class FlingStopFilterInterpreterTest : public ::testing::Test {};

namespace {

class FlingStopFilterInterpreterTestInterpreter : public Interpreter {
 public:
  FlingStopFilterInterpreterTestInterpreter()
      : Interpreter(NULL, NULL, false),
        sync_interpret_called_(false), handle_timer_called_(true),
        next_timeout_(-1) {}

  virtual void SyncInterpret(HardwareState* hwstate, stime_t* timeout) {
    sync_interpret_called_ = true;
    *timeout = next_timeout_;
  }

  virtual void HandleTimer(stime_t now, stime_t* timeout) {
    handle_timer_called_ = true;
    *timeout = next_timeout_;
  }

  bool sync_interpret_called_;
  bool handle_timer_called_;
  stime_t next_timeout_;
};


struct SimpleTestInputs {
  stime_t now;
  short touch_cnt;  // -1 for timer callback

  bool expected_call_next;
  stime_t next_timeout;  // -1 for none
  stime_t expected_local_deadline;
  stime_t expected_next_deadline;
  stime_t expected_timeout;
  bool expected_fling_stop_out;
};

}  // namespace {}

TEST(FlingStopFilterInterpreterTest, SimpleTest) {
  FlingStopFilterInterpreterTestInterpreter* base_interpreter =
      new FlingStopFilterInterpreterTestInterpreter;
  FlingStopFilterInterpreter interpreter(NULL, base_interpreter, NULL);
  TestInterpreterWrapper wrapper(&interpreter);

  const stime_t kTO = interpreter.fling_stop_timeout_.val_ = 0.03;
  const stime_t kED= interpreter.fling_stop_extra_delay_.val_ = 0.055;

  SimpleTestInputs inputs[] = {
    // timeout case
    { 0.01,        1,  true, -1, 0.01 + kTO, 0.0,        kTO, false },
    { 0.02,        1,  true, -1, 0.01 + kTO, 0.0, kTO - 0.01, false },
    { 0.03,        0,  true, -1, 0.01 + kTO, 0.0, kTO - 0.02, false },
    { 0.01 + kTO, -1, false, -1,        0.0, 0.0,       -1.0, true },

    // multiple fingers come down, timeout
    { 3.01,      1,  true, -1, 3.01 + kTO, 0.0,        kTO, false },
    { 3.02,      2,  true, -1, 3.01 + kTO + kED, 0.0, kTO + kED - 0.01, false },
    { 3.03,      0,  true, -1, 3.01 + kTO + kED, 0.0, kTO + kED - 0.02, false },
    { 3.01 + kTO + kED, -1, false, -1,        0.0, 0.0,       -1.0, true },

    // Dual timeouts, local is shorter
    { 6.01,        1,  true, -1.0, 6.01 + kTO,        0.0,        kTO, false },
    { 6.02,        0,  true,  0.1, 6.01 + kTO, 6.02 + 0.1, kTO - 0.01, false },
    { 6.01 + kTO, -1, false, -1.0,        0.0, 6.02 + 0.1,       0.08, true },
    { 6.02 + 0.1, -1,  true, -1.0,        0.0,        0.0,       -1.0, false },

    // Dual timeouts, local is longer
    { 9.01,        1,  true, -1.0, 9.01 + kTO,        0.0,       kTO, false },
    { 9.02,        0,  true,  .01, 9.01 + kTO, 9.02 + .01,       .01, false },
    { 9.02 + .01, -1,  true, -1.0, 9.01 + kTO, 0.0, kTO - .01 - 0.01, false },
    { 9.01 + kTO, -1, false, -1.0,        0.0,        0.0,      -1.0, true },

    // Dual timeouts, new timeout on handling timeout
    { 12.01,      1,  true, -1.0, 12.01 + kTO,         0.0,        kTO, false },
    { 12.02,      0,  true,  0.1, 12.01 + kTO, 12.02 + 0.1, kTO - 0.01, false },
    { 12.01 + kTO, -1, false, -1.0,       0.0, 12.02 + 0.1,       0.08, true },
    { 12.02 + 0.1, -1,  true, 0.1, 0.0, 12.22, 0.1, false },
    { 12.22, -1, true, -1.0, 0.0, 0.0, -1.0, false },

    // Overrun deadline
    { 15.01,        1,  true, -1, 15.01 + kTO, 0.0,        kTO, false },
    { 15.02,        1,  true, -1, 15.01 + kTO, 0.0, kTO - 0.01, false },
    { 15.03,        0,  true, -1, 15.01 + kTO, 0.0, kTO - 0.02, false },
    { 15.02 + kTO,  0,  true, -1,         0.0, 0.0,       -1.0, true },
  };

  for (size_t i = 0; i < arraysize(inputs); i++) {
    SimpleTestInputs& input = inputs[i];

    base_interpreter->sync_interpret_called_ = false;
    base_interpreter->handle_timer_called_ = false;
    base_interpreter->next_timeout_ = input.next_timeout;

    stime_t timeout = -1.0;

    Gesture* ret = NULL;
    if (input.touch_cnt >= 0) {
      FingerState fs[5];
      memset(fs, 0, sizeof(fs));
      unsigned short touch_cnt = static_cast<unsigned short>(input.touch_cnt);
      HardwareState hs = {
        input.now, 0, touch_cnt, touch_cnt, fs, 0, 0, 0, 0
      };

      ret = wrapper.SyncInterpret(&hs, &timeout);

      EXPECT_EQ(input.expected_call_next,
                base_interpreter->sync_interpret_called_) << "i=" << i;
      EXPECT_FALSE(base_interpreter->handle_timer_called_) << "i=" << i;
    } else {
      ret = wrapper.HandleTimer(input.now, &timeout);

      EXPECT_EQ(input.expected_call_next,
                base_interpreter->handle_timer_called_) << "i=" << i;
      EXPECT_FALSE(base_interpreter->sync_interpret_called_) << "i=" << i;
    }
    EXPECT_FLOAT_EQ(input.expected_local_deadline,
                    interpreter.fling_stop_deadline_) << "i=" << i;
    EXPECT_FLOAT_EQ(input.expected_next_deadline,
                    interpreter.next_timer_deadline_) << "i=" << i;
    EXPECT_FLOAT_EQ(input.expected_timeout, timeout) << "i=" << i;
    EXPECT_EQ(input.expected_fling_stop_out,
              ret && ret->type == kGestureTypeFling &&
              ret->details.fling.fling_state == GESTURES_FLING_TAP_DOWN)
        << "i=" << i;
  }
}

}  // namespace gestures
