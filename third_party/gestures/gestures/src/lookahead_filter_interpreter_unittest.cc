// Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <deque>
#include <math.h>
#include <set>
#include <stdio.h>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "gestures/include/gestures.h"
#include "gestures/include/lookahead_filter_interpreter.h"
#include "gestures/include/unittest_util.h"
#include "gestures/include/util.h"

using std::deque;
using std::pair;

namespace gestures {

class LookaheadFilterInterpreterTest : public ::testing::Test {};

class LookaheadFilterInterpreterTestInterpreter : public Interpreter {
 public:
  LookaheadFilterInterpreterTestInterpreter()
      : Interpreter(NULL, NULL, false),
        timer_return_(-1.0),
        clear_incoming_hwstates_(false), expected_id_(-1),
        expected_flags_(0), expected_flags_at_(-1),
        expected_flags_at_occurred_(false) {}

  virtual void SyncInterpret(HardwareState* hwstate, stime_t* timeout) {
    for (size_t i = 0; i < hwstate->finger_cnt; i++)
      all_ids_.insert(hwstate->fingers[i].tracking_id);
    if (expected_id_ >= 0) {
      EXPECT_EQ(1, hwstate->finger_cnt);
      EXPECT_EQ(expected_id_, hwstate->fingers[0].tracking_id);
    }
    if (!expected_ids_.empty()) {
      EXPECT_EQ(expected_ids_.size(), hwstate->finger_cnt);
      for (set<short, kMaxFingers>::iterator it = expected_ids_.begin(),
               e = expected_ids_.end(); it != e; ++it) {
        EXPECT_TRUE(hwstate->GetFingerState(*it)) << "Can't find ID " << *it
                                                  << " at "
                                                  << hwstate->timestamp;
      }
    }
    if (expected_flags_at_ >= 0 &&
        DoubleEq(expected_flags_at_, hwstate->timestamp) &&
        hwstate->finger_cnt > 0) {
      EXPECT_EQ(expected_flags_, hwstate->fingers[0].flags);
      expected_flags_at_occurred_ = true;
    }
    if (clear_incoming_hwstates_)
      hwstate->finger_cnt = 0;
    if (timer_return_ >= 0.0) {
      *timeout = timer_return_;
      timer_return_ = -1.0;
    }
    if (return_values_.empty())
      return;
    return_value_ = return_values_.front();
    return_values_.pop_front();
    ProduceGesture(return_value_);
  }

  virtual void HandleTimer(stime_t now, stime_t* timeout) {
    EXPECT_TRUE(false);
  }

  Gesture return_value_;
  deque<Gesture> return_values_;
  stime_t timer_return_;
  bool clear_incoming_hwstates_;
  // if expected_id_ >= 0, we expect that there is one finger with
  // the expected id.
  short expected_id_;
  // if expected_ids_ is populated, we expect fingers w/ exactly those IDs
  set<short, kMaxFingers> expected_ids_;
  // If we get a hardware state at time expected_flags_at_, it should have
  // the following flags set.
  unsigned expected_flags_;
  stime_t expected_flags_at_;
  bool expected_flags_at_occurred_;
  std::set<short> all_ids_;
};

TEST(LookaheadFilterInterpreterTest, SimpleTest) {
  LookaheadFilterInterpreterTestInterpreter* base_interpreter = NULL;
  std::unique_ptr<LookaheadFilterInterpreter> interpreter;

  HardwareProperties initial_hwprops = {
    0, 0, 100, 100,  // left, top, right, bottom
    10,  // x res (pixels/mm)
    10,  // y res (pixels/mm)
    133, 133,  // scrn DPI X, Y
    -1,  // orientation minimum
    2,   // orientation maximum
    2, 5,  // max fingers, max_touch
    1, 0, 0, 0  // t5r2, semi, button pad
  };
  TestInterpreterWrapper wrapper(interpreter.get(), &initial_hwprops);

  FingerState fs[] = {
    // TM, Tm, WM, Wm, pr, orient, x, y, id
    { 0, 0, 0, 0, 1, 0, 10, 1, 1, 0 },
    { 0, 0, 0, 0, 1, 0, 10, 2, 1, 0 },
    { 0, 0, 0, 0, 1, 0, 10, 3, 1, 0 },

    { 0, 0, 0, 0, 1, 0, 10, 1, 2, 0 },
    { 0, 0, 0, 0, 1, 0, 10, 2, 2, 0 },

    { 0, 0, 0, 0, 1, 0, 10, 1, 3, 0 },
    { 0, 0, 0, 0, 1, 0, 10, 2, 3, 0 },
  };
  HardwareState hs[] = {
    // Expect movement to take
    { 1.01, 0, 1, 1, &fs[0], 0, 0, 0, 0 },
    { 1.02, 0, 1, 1, &fs[1], 0, 0, 0, 0 },
    { 1.03, 0, 1, 1, &fs[2], 0, 0, 0, 0 },

    // Expect no movement
    { 2.010, 0, 1, 1, &fs[3], 0, 0, 0, 0 },
    { 2.030, 0, 1, 1, &fs[4], 0, 0, 0, 0 },
    { 2.031, 0, 0, 0, NULL, 0, 0, 0, 0 },

    // Expect movement b/c it's moving really fast
    { 3.010, 0, 1, 1, &fs[5], 0, 0, 0, 0 },
    { 3.011, 0, 1, 1, &fs[6], 0, 0, 0, 0 },
    { 3.030, 0, 0, 0, NULL, 0, 0, 0, 0 }
  };

  stime_t expected_timeout = 0.0;
  Gesture expected_movement;
  for (size_t i = 3; i < arraysize(hs); ++i) {
    if (i % 3 == 0) {
      base_interpreter = new LookaheadFilterInterpreterTestInterpreter;

      for (size_t j = 0; j < 2; ++j) {
        if (hs[i + j + 1].finger_cnt == 0)
          break;
        expected_movement = Gesture(kGestureMove,
                                    hs[i + j].timestamp,  // start time
                                    hs[i + j + 1].timestamp,  // end time
                                    hs[i + j + 1].fingers[0].position_x -
                                    hs[i + j].fingers[0].position_x,  // dx
                                    hs[i + j + 1].fingers[0].position_y -
                                    hs[i + j].fingers[0].position_y);  // dy
        base_interpreter->return_values_.push_back(expected_movement);
      }

      interpreter.reset(new LookaheadFilterInterpreter(
          NULL, base_interpreter, NULL));
      wrapper.Reset(interpreter.get());
      interpreter->min_delay_.val_ = 0.05;
      expected_timeout = interpreter->min_delay_.val_;
    }
    stime_t timeout = -1.0;
    Gesture* out = wrapper.SyncInterpret(&hs[i], &timeout);
    if (out) {
      EXPECT_EQ(kGestureTypeFling, out->type);
      EXPECT_EQ(GESTURES_FLING_TAP_DOWN, out->details.fling.fling_state);
    }
    EXPECT_LT(fabs(expected_timeout - timeout), 0.0000001);
    if ((i % 3) != 2) {
      expected_timeout -= hs[i + 1].timestamp - hs[i].timestamp;
    } else {
      stime_t newtimeout = -1.0;
      out = wrapper.HandleTimer(hs[i].timestamp + timeout, &newtimeout);
      if (newtimeout < 0.0)
        EXPECT_DOUBLE_EQ(-1.0, newtimeout);
      if (i == 5) {
        EXPECT_EQ(reinterpret_cast<Gesture*>(NULL), out);
      } else {
        // Expect movement
        ASSERT_TRUE(out);
        EXPECT_EQ(kGestureTypeMove, out->type);
        EXPECT_EQ(expected_movement.start_time, out->start_time);
        EXPECT_EQ(expected_movement.end_time, out->end_time);
        EXPECT_EQ(expected_movement.details.move.dx, out->details.move.dx);
        EXPECT_EQ(expected_movement.details.move.dy, out->details.move.dy);
      }
      // Run through rest of interpreter timeouts, makeing sure we get
      // reasonable timeout values
      int cnt = 0;
      stime_t now = hs[i].timestamp + timeout;
      while (newtimeout >= 0.0) {
        if (cnt++ == 10)
          break;
        timeout = newtimeout;
        newtimeout = -1.0;
        now += timeout;
        out = wrapper.HandleTimer(now, &newtimeout);
        if (newtimeout >= 0.0)
          EXPECT_LT(newtimeout, 1.0);
        else
          EXPECT_DOUBLE_EQ(-1.0, newtimeout);
      }
    }
  }
}

class LookaheadFilterInterpreterVariableDelayTestInterpreter
    : public Interpreter {
 public:
  LookaheadFilterInterpreterVariableDelayTestInterpreter()
      : Interpreter(NULL, NULL, false), interpret_call_count_ (0) {}

  virtual void SyncInterpret(HardwareState* hwstate, stime_t* timeout) {
    interpret_call_count_++;
    EXPECT_EQ(1, hwstate->finger_cnt);
    finger_ids_.insert(hwstate->fingers[0].tracking_id);
  }

  virtual void HandleTimer(stime_t now, stime_t* timeout) {
    EXPECT_TRUE(false);
  }

  std::set<short> finger_ids_;
  size_t interpret_call_count_;
};

// Tests that with a zero delay, we can still avoid unnecessary splitting
// by using variable delay.
TEST(LookaheadFilterInterpreterTest, VariableDelayTest) {
  LookaheadFilterInterpreterVariableDelayTestInterpreter* base_interpreter =
      new LookaheadFilterInterpreterVariableDelayTestInterpreter;
  LookaheadFilterInterpreter interpreter(NULL, base_interpreter, NULL);

  HardwareProperties initial_hwprops = {
    0, 0, 100, 100,  // left, top, right, bottom
    1,  // x res (pixels/mm)
    1,  // y res (pixels/mm)
    133, 133,  // scrn DPI X, Y
    -1,  // orientation minimum
    2,   // orientation maximum
    5, 5,  // max fingers, max_touch,
    0, 0, 0, 0  // t5r2, semi, button pad
  };
  TestInterpreterWrapper wrapper(&interpreter, &initial_hwprops);

  FingerState fs[] = {
    // TM, Tm, WM, Wm, pr, orient, x, y, id
    { 0, 0, 0, 0, 1, 0, 10, 10, 10, 1 },
    { 0, 0, 0, 0, 1, 0, 10, 30, 10, 1 },
    { 0, 0, 0, 0, 1, 0, 10, 50, 10, 1 },
  };
  HardwareState hs[] = {
    // Expect movement to take
    { 1.01, 0, 1, 1, &fs[0], 0, 0, 0, 0 },
    { 1.02, 0, 1, 1, &fs[1], 0, 0, 0, 0 },
    { 1.03, 0, 1, 1, &fs[2], 0, 0, 0, 0 },
  };

  interpreter.min_delay_.val_ = 0.0;

  for (size_t i = 0; i < arraysize(hs); i++) {
    stime_t timeout = -1.0;
    wrapper.SyncInterpret(&hs[i], &timeout);
    stime_t next_input = i < (arraysize(hs) - 1) ? hs[i + 1].timestamp :
        INFINITY;
    stime_t now = hs[i].timestamp;
    while (timeout >= 0 && (timeout + now) < next_input) {
      now += timeout;
      timeout = -1.0;
      wrapper.HandleTimer(now, &timeout);
    }
  }
  EXPECT_EQ(3, base_interpreter->interpret_call_count_);
  EXPECT_EQ(1, base_interpreter->finger_ids_.size());
  EXPECT_EQ(1, *base_interpreter->finger_ids_.begin());
}

class LookaheadFilterInterpreterNoTapSetTestInterpreter
    : public Interpreter {
 public:
  LookaheadFilterInterpreterNoTapSetTestInterpreter()
      : Interpreter(NULL, NULL, false), interpret_call_count_(0) {}

  virtual void SyncInterpret(HardwareState* hwstate, stime_t* timeout) {
    EXPECT_EQ(expected_finger_cnts_[interpret_call_count_],
              hwstate->finger_cnt);
    interpret_call_count_++;
    if (hwstate->finger_cnt > 0)
      EXPECT_TRUE(hwstate->fingers[0].flags & GESTURES_FINGER_NO_TAP);
  }

  virtual void HandleTimer(stime_t now, stime_t* timeout) {
    EXPECT_TRUE(false);
  }

  std::set<short> finger_ids_;
  size_t interpret_call_count_;
  std::vector<short> expected_finger_cnts_;
};

// Tests that with a zero delay, we can still avoid unnecessary splitting
// by using variable delay.
TEST(LookaheadFilterInterpreterTest, NoTapSetTest) {
  LookaheadFilterInterpreterNoTapSetTestInterpreter* base_interpreter =
      new LookaheadFilterInterpreterNoTapSetTestInterpreter;
  LookaheadFilterInterpreter interpreter(NULL, base_interpreter, NULL);
  interpreter.min_delay_.val_ = 0.0;

  HardwareProperties initial_hwprops = {
    0, 0, 100, 100,  // left, top, right, bottom
    1,  // x res (pixels/mm)
    1,  // y res (pixels/mm)
    133, 133,  // scrn DPI X, Y
    -1,  // orientation minimum
    2,   // orientation maximum
    5, 5,  // max fingers, max_touch
    0, 0, 0, 0  // t5r2, semi, button pad
  };

  FingerState fs[] = {
    // TM, Tm, WM, Wm, pr, orient, x, y, id, flags
    { 0, 0, 0, 0, 1, 0, 30, 10, 0, 0 },
    { 0, 0, 0, 0, 1, 0, 30, 30, 0, 0 },
    { 0, 0, 0, 0, 1, 0, 10, 10, 1, 0 },
    { 0, 0, 0, 0, 1, 0, 10, 30, 1, 0 },
  };
  HardwareState hs[] = {
    // Expect movement to take
    { 0.01, 0, 1, 1, &fs[0], 0, 0, 0, 0 },
    { 0.01, 0, 1, 1, &fs[1], 0, 0, 0, 0 },
    { 0.03, 0, 0, 0, NULL, 0, 0, 0, 0 },
    { 1.01, 0, 1, 1, &fs[2], 0, 0, 0, 0 },
    { 1.02, 0, 1, 1, &fs[3], 0, 0, 0, 0 },
  };

  TestInterpreterWrapper wrapper(&interpreter, &initial_hwprops);


  for (size_t i = 0; i < arraysize(hs); i++) {
    base_interpreter->expected_finger_cnts_.push_back(hs[i].finger_cnt);
    stime_t timeout = -1.0;
    interpreter.SyncInterpret(&hs[i], &timeout);
    stime_t next_input = i < (arraysize(hs) - 1) ? hs[i + 1].timestamp :
        INFINITY;
    stime_t now = hs[i].timestamp;
    while (timeout >= 0 && (timeout + now) < next_input) {
      now += timeout;
      timeout = -1.0;
      interpreter.HandleTimer(now, &timeout);
    }
  }
  EXPECT_EQ(arraysize(hs), base_interpreter->interpret_call_count_);
}

// This test makes sure that if an interpreter requests a timeout, and then
// there is a spurious callback, that we request another callback for the time
// that remains.
TEST(LookaheadFilterInterpreterTest, SpuriousCallbackTest) {
  LookaheadFilterInterpreterTestInterpreter* base_interpreter = NULL;
  std::unique_ptr<LookaheadFilterInterpreter> interpreter;

  HardwareProperties initial_hwprops = {
    0, 0, 100, 100,  // left, top, right, bottom
    10,  // x res (pixels/mm)
    10,  // y res (pixels/mm)
    133, 133,  // scrn DPI X, Y
    -1,  // orientation minimum
    2,   // orientation maximum
    2, 5,  // max fingers, max_touch
    1, 0, 0, 0  // t5r2, semi, button pad
  };
  TestInterpreterWrapper wrapper(interpreter.get(), &initial_hwprops);

  HardwareState hs = {1, 0, 0, 0, NULL, 0, 0, 0, 0};

  base_interpreter = new LookaheadFilterInterpreterTestInterpreter;
  base_interpreter->timer_return_ = 1.0;
  interpreter.reset(new LookaheadFilterInterpreter(
      NULL, base_interpreter, NULL));
  wrapper.Reset(interpreter.get());
  interpreter->min_delay_.val_ = 0.05;

  stime_t timeout = -1.0;
  Gesture* out = wrapper.SyncInterpret(&hs, &timeout);
  EXPECT_EQ(reinterpret_cast<Gesture*>(NULL), out);
  EXPECT_FLOAT_EQ(interpreter->min_delay_.val_, timeout);

  out = wrapper.HandleTimer(hs.timestamp + interpreter->min_delay_.val_,
                                 &timeout);
  EXPECT_EQ(reinterpret_cast<Gesture*>(NULL), out);
  EXPECT_FLOAT_EQ(1.0, timeout);


  out = wrapper.HandleTimer(hs.timestamp + interpreter->min_delay_.val_ +
                                 0.25,
                                 &timeout);
  EXPECT_EQ(reinterpret_cast<Gesture*>(NULL), out);
  EXPECT_FLOAT_EQ(0.75, timeout);
}

TEST(LookaheadFilterInterpreterTest, TimeGoesBackwardsTest) {
  LookaheadFilterInterpreterTestInterpreter* base_interpreter =
      new LookaheadFilterInterpreterTestInterpreter;
  Gesture expected_movement = Gesture(kGestureMove,
                                      0.0,  // start time
                                      0.0,  // end time
                                      1.0,  // dx
                                      1.0);  // dy
  base_interpreter->return_values_.push_back(expected_movement);
  base_interpreter->return_values_.push_back(expected_movement);
  LookaheadFilterInterpreter interpreter(NULL, base_interpreter, NULL);

  HardwareProperties initial_hwprops = {
    0, 0, 100, 100,  // left, top, right, bottom
    1,  // x res (pixels/mm)
    1,  // y res (pixels/mm)
    133, 133,  // scrn DPI X, Y
    -1,  // orientation minimum
    2,   // orientation maximum
    2, 5,  // max fingers, max_touch
    1, 0, 0, 0  // t5r2, semi, button pad
  };
  TestInterpreterWrapper wrapper(&interpreter, &initial_hwprops);


  FingerState fs = {
    // TM, Tm, WM, Wm, pr, orient, x, y, id
    0, 0, 0, 0, 1, 0, 20, 20, 1, 0
  };
  HardwareState hs[] = {
    // Initial state
    { 9.00, 0, 1, 1, &fs, 0, 0, 0, 0 },
    // Time jumps backwards, then goes forwards
    { 0.01, 0, 1, 1, &fs, 0, 0, 0, 0 },
    { 0.02, 0, 1, 1, &fs, 0, 0, 0, 0 },
    { 0.03, 0, 1, 1, &fs, 0, 0, 0, 0 },
    { 0.04, 0, 1, 1, &fs, 0, 0, 0, 0 },
    { 0.05, 0, 1, 1, &fs, 0, 0, 0, 0 },
    { 0.06, 0, 1, 1, &fs, 0, 0, 0, 0 },
    { 0.07, 0, 1, 1, &fs, 0, 0, 0, 0 },
    { 0.08, 0, 1, 1, &fs, 0, 0, 0, 0 },
    { 0.09, 0, 1, 1, &fs, 0, 0, 0, 0 },
    { 0.10, 0, 1, 1, &fs, 0, 0, 0, 0 },
    { 0.11, 0, 1, 1, &fs, 0, 0, 0, 0 },
    { 0.12, 0, 1, 1, &fs, 0, 0, 0, 0 },
    { 0.13, 0, 1, 1, &fs, 0, 0, 0, 0 },
    { 0.14, 0, 1, 1, &fs, 0, 0, 0, 0 },
    { 0.15, 0, 1, 1, &fs, 0, 0, 0, 0 },
    { 0.16, 0, 1, 1, &fs, 0, 0, 0, 0 },
    { 0.17, 0, 1, 1, &fs, 0, 0, 0, 0 },
    { 0.18, 0, 1, 1, &fs, 0, 0, 0, 0 },
    { 0.19, 0, 1, 1, &fs, 0, 0, 0, 0 },
    { 0.20, 0, 1, 1, &fs, 0, 0, 0, 0 }
  };
  for (size_t i = 0; i < arraysize(hs); ++i) {
    stime_t timeout_requested = -1.0;
    Gesture* result = wrapper.SyncInterpret(&hs[i], &timeout_requested);
    if (result && result->type == kGestureTypeMove)
      return;  // Success!
  }
  ADD_FAILURE() << "Should have gotten a move gesture";
}

TEST(LookaheadFilterInterpreterTest, InterpolateHwStateTest) {
  // This test takes the first two HardwareStates, Interpolates them, putting
  // the output into the fourth slot. The third slot is the expected output.
  FingerState fs[] = {
    // TM, Tm, WM, Wm, pr, orient, x, y, id
    { 0.1, 0.4, 1.6, 1.2, 10, 3, 102, 102, 1, 0 },
    { 0.2, 0.5, 1.7, 1.3, 11, 4, 4, 4, 2, 0 },
    { 0.3, 0.6, 1.8, 1.4, 12, 5, 4444, 9999, 3, 0 },

    { 0.5, 0.2, 2.0, 1.2, 13, 8, 200, 100, 1, 0 },
    { 0.7, 0.4, 2.3, 1.3, 17, 7, 20, 22, 2, 0 },
    { 1.0, 0.5, 2.4, 1.6, 10, 9, 5000, 5000, 3, 0 },

    { 0.3, 0.3, 1.8, 1.2, 11.5, 5.5, 151, 101, 1, 0 },
    { 0.45, 0.45, 2.0, 1.3, 14, 5.5, 12, 13, 2, 0 },
    { 0.65, 0.55, 2.1, 1.5, 11, 7, 4722, 7499.5, 3, 0 },

    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
  };
  HardwareState hs[] = {
    // Expect movement to take
    { 1.011,  2, 3, 3, &fs[0], 0, 0, 0, 0 },
    { 1.022,  2, 3, 3, &fs[3], 0, 0, 0, 0 },
    { 1.0165, 2, 3, 3, &fs[6], 0, 0, 0, 0 },
    { 0, 0, 0, 0, &fs[9], 0, 0, 0, 0 },
  };

  LookaheadFilterInterpreter::Interpolate(hs[0], hs[1], &hs[3]);
  EXPECT_DOUBLE_EQ(hs[2].timestamp, hs[3].timestamp);
  EXPECT_EQ(hs[2].buttons_down, hs[3].buttons_down);
  EXPECT_EQ(hs[2].touch_cnt, hs[3].touch_cnt);
  EXPECT_EQ(hs[2].finger_cnt, hs[3].finger_cnt);
  for (size_t i = 0; i < hs[3].finger_cnt; i++)
    EXPECT_TRUE(hs[2].fingers[i] == hs[3].fingers[i]) << "i=" << i;
}

TEST(LookaheadFilterInterpreterTest, InterpolateTest) {
  LookaheadFilterInterpreterTestInterpreter* base_interpreter = NULL;
  std::unique_ptr<LookaheadFilterInterpreter> interpreter;

  HardwareProperties initial_hwprops = {
    0, 0, 100, 100,  // left, top, right, bottom
    10,  // x res (pixels/mm)
    10,  // y res (pixels/mm)
    133, 133,  // scrn DPI X, Y
    -1,  // orientation minimum
    2,   // orientation maximum
    2, 5,  // max fingers, max_touch
    1, 0, 0, 0  // t5r2, semi, button pad
  };
  TestInterpreterWrapper wrapper(interpreter.get(), &initial_hwprops);

  FingerState fs = {
    // TM, Tm, WM, Wm, pr, orient, x, y, id
    0, 0, 0, 0, 1, 0, 10, 1, 1, 0
  };
  HardwareState hs[] = {
    // Expect movement to take
    { 1.01, 0, 1, 1, &fs, 0, 0, 0, 0 },
    { 1.02, 0, 1, 1, &fs, 0, 0, 0, 0 },
    { 1.04, 0, 1, 1, &fs, 0, 0, 0, 0 },
  };

  // Tests that we can properly decide when to interpolate two events.
  for (size_t i = 0; i < 2; ++i) {
    bool should_interpolate = i;
    base_interpreter = new LookaheadFilterInterpreterTestInterpreter;
    base_interpreter->clear_incoming_hwstates_ = true;
    base_interpreter->return_values_.push_back(
        Gesture(kGestureMove,
                0,  // start time
                1,  // end time
                0,  // dx
                1));  // dy
    base_interpreter->return_values_.push_back(
        Gesture(kGestureMove,
                1,  // start time
                2,  // end time
                0,  // dx
                2));  // dy
    base_interpreter->return_values_.push_back(
        Gesture(kGestureMove,
                2,  // start time
                3,  // end time
                0,  // dx
                3));  // dy
    interpreter.reset(new LookaheadFilterInterpreter(
        NULL, base_interpreter, NULL));
    wrapper.Reset(interpreter.get());
    interpreter->min_delay_.val_ = 0.05;

    stime_t timeout = -1.0;
    Gesture* out = wrapper.SyncInterpret(&hs[0], &timeout);
    EXPECT_EQ(reinterpret_cast<Gesture*>(NULL), out);
    EXPECT_GT(timeout, 0);
    const size_t next_idx = should_interpolate ? 2 : 1;
    timeout = -1.0;
    out = wrapper.SyncInterpret(&hs[next_idx], &timeout);
    EXPECT_EQ(reinterpret_cast<Gesture*>(NULL), out);
    EXPECT_GT(timeout, 0);

    // Fetch the gestures
    size_t gs_count = 0;
    stime_t now = hs[next_idx].timestamp + timeout;
    do {
      timeout = -1.0;
      out = wrapper.HandleTimer(now, &timeout);
      EXPECT_NE(reinterpret_cast<Gesture*>(NULL), out);
      gs_count++;
      now += timeout;
    } while(timeout > 0.0);
    EXPECT_EQ(should_interpolate ? 3 : 2, gs_count);
  }
}

TEST(LookaheadFilterInterpreterTest, InterpolationOverdueTest) {
  LookaheadFilterInterpreterTestInterpreter* base_interpreter = NULL;
  std::unique_ptr<LookaheadFilterInterpreter> interpreter;

  HardwareProperties initial_hwprops = {
    0, 0, 10, 10,  // left, top, right, bottom
    1,  // x res (pixels/mm)
    1,  // y res (pixels/mm)
    25, 25,  // scrn DPI X, Y
    -1,  // orientation minimum
    2,   // orientation maximum
    2, 5,  // max fingers, max_touch
    1, 0, 0, 0  // t5r2, semi, button pad
  };
  TestInterpreterWrapper wrapper(interpreter.get(), &initial_hwprops);

  FingerState fs = {
    // TM, Tm, WM, Wm, pr, orient, x, y, id
    0, 0, 0, 0, 1, 0, 10, 1, 1, 0
  };
  // These timestamps cause an interpolated event to be 1.492 at time 1.495,
  // and so this tests that an overdue interpolated event is handled correctly.
  HardwareState hs[] = {
    // Expect movement to take
    { 1.456, 0, 1, 1, &fs, 0, 0, 0, 0 },
    { 1.495, 0, 1, 1, &fs, 0, 0, 0, 0 },
  };

  base_interpreter = new LookaheadFilterInterpreterTestInterpreter;
  base_interpreter->timer_return_ = 0.700;
  base_interpreter->return_values_.push_back(
      Gesture(kGestureMove,
              0,  // start time
              1,  // end time
              0,  // dx
              1));  // dy
  base_interpreter->return_values_.push_back(
      Gesture(kGestureMove,
              1,  // start time
              2,  // end time
              0,  // dx
              2));  // dy
  interpreter.reset(new LookaheadFilterInterpreter(
      NULL, base_interpreter, NULL));
  interpreter->min_delay_.val_ = 0.017;
  wrapper.Reset(interpreter.get());

  stime_t timeout = -1.0;
  Gesture* out = wrapper.SyncInterpret(&hs[0], &timeout);
  EXPECT_EQ(reinterpret_cast<Gesture*>(NULL), out);
  EXPECT_FLOAT_EQ(timeout, interpreter->min_delay_.val_);

  stime_t now = hs[0].timestamp + timeout;
  timeout = -1.0;
  out = wrapper.HandleTimer(now, &timeout);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), out);
  EXPECT_EQ(kGestureTypeMove, out->type);
  EXPECT_EQ(1, out->details.move.dy);
  EXPECT_DOUBLE_EQ(timeout, 0.700);

  timeout = -1.0;
  out = wrapper.SyncInterpret(&hs[1], &timeout);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), out);
  EXPECT_EQ(kGestureTypeMove, out->type);
  EXPECT_EQ(2, out->details.move.dy);
  EXPECT_GE(timeout, 0.0);
}

struct HardwareStateLastId {
  HardwareState hs;
  short expected_last_id;
};

TEST(LookaheadFilterInterpreterTest, DrumrollTest) {
  LookaheadFilterInterpreterTestInterpreter* base_interpreter = NULL;
  std::unique_ptr<LookaheadFilterInterpreter> interpreter;

  HardwareProperties initial_hwprops = {
    0, 0, 100, 100,  // left, top, right, bottom
    1,  // x res (pixels/mm)
    1,  // y res (pixels/mm)
    25, 25,  // scrn DPI X, Y
    -1,  // orientation minimum
    2,   // orientation maximum
    2, 5,  // max fingers, max_touch
    1, 0, 0, 0  // t5r2, semi, button pad
  };
  TestInterpreterWrapper wrapper(interpreter.get(), &initial_hwprops);

  FingerState fs[] = {
    // TM, Tm, WM, Wm, pr, orient, x, y, id
    { 0, 0, 0, 0, 1, 0, 40, 40, 1, 0 },
    { 0, 0, 0, 0, 1, 0, 40, 80, 1, 0 },
    { 0, 0, 0, 0, 1, 0, 40, 40, 2, 0 },
    { 0, 0, 0, 0, 1, 0, 41, 80, 2, 0 },
    // Warp cases:
    { 0, 0, 0, 0, 1, 0, 40, 40, 3, 0 },
    { 0, 0, 0, 0, 1, 0, 41, 80, 3, GESTURES_FINGER_WARP_X },
    { 0, 0, 0, 0, 1, 0, 40, 40, 4, 0 },
    { 0, 0, 0, 0, 1, 0, 41, 80, 4, GESTURES_FINGER_WARP_Y },
    { 0, 0, 0, 0, 1, 0, 40, 40, 5, 0 },
    { 0, 0, 0, 0, 1, 0, 41, 80, 5, GESTURES_FINGER_WARP_X |
                                   GESTURES_FINGER_WARP_Y },
  };
  // These timestamps cause an interpolated event to be 1.492 at time 1.495,
  // and so this tests that an overdue interpolated event is handled correctly.
  HardwareStateLastId hsid[] = {
    // Expect movement to take
    { { 1.000, 0, 1, 1, &fs[0], 0, 0, 0, 0 }, 1 },
    { { 1.001, 0, 1, 1, &fs[0], 0, 0, 0, 0 }, 1 },
    { { 1.002, 0, 1, 1, &fs[1], 0, 0, 0, 0 }, 2 },
    { { 1.003, 0, 1, 1, &fs[1], 0, 0, 0, 0 }, 2 },
    { { 1.004, 0, 1, 1, &fs[2], 0, 0, 0, 0 }, 3 },
    { { 1.005, 0, 1, 1, &fs[3], 0, 0, 0, 0 }, 4 },
    { { 1.006, 0, 1, 1, &fs[2], 0, 0, 0, 0 }, 5 },
    // Warp cases:
    { { 1.007, 0, 1, 1, &fs[4], 0, 0, 0, 0 }, 6 },
    { { 1.008, 0, 1, 1, &fs[5], 0, 0, 0, 0 }, 6 },
    { { 1.009, 0, 1, 1, &fs[6], 0, 0, 0, 0 }, 7 },
    { { 1.010, 0, 1, 1, &fs[7], 0, 0, 0, 0 }, 7 },
    { { 1.011, 0, 1, 1, &fs[8], 0, 0, 0, 0 }, 8 },
    { { 1.012, 0, 1, 1, &fs[9], 0, 0, 0, 0 }, 8 },
  };

  base_interpreter = new LookaheadFilterInterpreterTestInterpreter;
  base_interpreter->return_values_.push_back(
      Gesture(kGestureMove,
              0,  // start time
              1,  // end time
              0,  // dx
              1));  // dy
  interpreter.reset(new LookaheadFilterInterpreter(
      NULL, base_interpreter, NULL));
  wrapper.Reset(interpreter.get());

  for (size_t i = 0; i < arraysize(hsid); i++) {
    stime_t timeout = -1.0;
    Gesture* out = wrapper.SyncInterpret(&hsid[i].hs, &timeout);
    if (out) {
      EXPECT_EQ(kGestureTypeFling, out->type);
      EXPECT_EQ(GESTURES_FLING_TAP_DOWN, out->details.fling.fling_state);
    }
    EXPECT_GT(timeout, 0);
    EXPECT_EQ(interpreter->last_id_, hsid[i].expected_last_id);
  }
}

TEST(LookaheadFilterInterpreterTest, QuickMoveTest) {
  LookaheadFilterInterpreterTestInterpreter* base_interpreter = NULL;
  std::unique_ptr<LookaheadFilterInterpreter> interpreter;

  HardwareProperties initial_hwprops = {
    0, 0, 100, 100,  // left, top, right, bottom
    1,  // x res (pixels/mm)
    1,  // y res (pixels/mm)
    25, 25,  // scrn DPI X, Y
    -1,  // orientation minimum
    2,   // orientation maximum
    2, 5,  // max fingers, max_touch
    1, 0, 0, 0  // t5r2, semi, button pad
  };
  TestInterpreterWrapper wrapper(interpreter.get(), &initial_hwprops);

  FingerState fs[] = {
    // TM, Tm, WM, Wm, pr, orient, x, y, id
    { 0, 0, 0, 0, 1, 0, 40, 40, 1, 0 },
    { 0, 0, 0, 0, 1, 0, 41, 80, 1, 0 },
    { 0, 0, 0, 0, 1, 0, 40, 40, 1, 0 },

    { 0, 0, 0, 0, 1, 0, 40, 40, 2, 0 },
    { 0, 0, 0, 0, 1, 0, 41, 80, 2, 0 },
    { 0, 0, 0, 0, 1, 0, 40, 120, 2, 0 },
  };

  HardwareState hs[] = {
    // Drumroll
    { 1.000, 0, 1, 1, &fs[0], 0, 0, 0, 0 },
    { 1.001, 0, 1, 1, &fs[1], 0, 0, 0, 0 },
    { 1.002, 0, 1, 1, &fs[2], 0, 0, 0, 0 },
    // No touch
    { 1.003, 0, 0, 0, &fs[0], 0, 0, 0, 0 },
    // Quick movement
    { 1.034, 0, 1, 1, &fs[3], 0, 0, 0, 0 },
    { 1.035, 0, 1, 1, &fs[4], 0, 0, 0, 0 },
    { 1.036, 0, 1, 1, &fs[5], 0, 0, 0, 0 },
  };

  base_interpreter = new LookaheadFilterInterpreterTestInterpreter;
  interpreter.reset(new LookaheadFilterInterpreter(
      NULL, base_interpreter, NULL));
  wrapper.Reset(interpreter.get());

  stime_t timeout = -1.0;
  List<LookaheadFilterInterpreter::QState>* queue = &interpreter->queue_;

  // Pushing the first event
  wrapper.SyncInterpret(&hs[0], &timeout);
  EXPECT_EQ(queue->size(), 1);
  EXPECT_EQ(queue->Tail()->fs_[0].tracking_id, 1);

  // Expecting Drumroll detected and ID reassigned 1 -> 2.
  wrapper.SyncInterpret(&hs[1], &timeout);
  EXPECT_EQ(queue->size(), 2);
  EXPECT_EQ(queue->Tail()->fs_[0].tracking_id, 2);

  // Expecting Drumroll detected and ID reassigned 1 -> 3.
  wrapper.SyncInterpret(&hs[2], &timeout);
  EXPECT_EQ(queue->size(), 3);
  EXPECT_EQ(queue->Tail()->fs_[0].tracking_id, 3);

  // Removing the touch.
  wrapper.SyncInterpret(&hs[3], &timeout);
  EXPECT_EQ(queue->size(), 4);

  // New event comes, old events removed from the queue.
  // New finger tracking ID assigned 2 - > 4.
  wrapper.SyncInterpret(&hs[4], &timeout);
  EXPECT_EQ(queue->size(), 2);
  EXPECT_EQ(queue->Tail()->fs_[0].tracking_id, 4);

  // Expecting Drumroll detected and ID reassigned 2 -> 5.
  wrapper.SyncInterpret(&hs[5], &timeout);
  EXPECT_EQ(queue->Tail()->fs_[0].tracking_id, 5);

  // Expecting Quick movement detected and ID correction 5 -> 4.
  wrapper.SyncInterpret(&hs[6], &timeout);
  EXPECT_EQ(queue->Tail()->fs_[0].tracking_id, 4);
  EXPECT_EQ(queue->Tail()->prev_->fs_[0].tracking_id, 4);
  EXPECT_EQ(queue->Tail()->prev_->prev_->fs_[0].tracking_id, 4);
}

struct QuickSwipeTestInputs {
  stime_t now;
  float x0, y0, pressure0;
  short id0;
  float x1, y1, pressure1;
  short id1;
};

// Based on a couple quick swipes of 2 fingers on Alex, makes sure that we
// don't drumroll-separate the fingers.
TEST(LookaheadFilterInterpreterTest, QuickSwipeTest) {
  LookaheadFilterInterpreterTestInterpreter* base_interpreter = NULL;
  std::unique_ptr<LookaheadFilterInterpreter> interpreter;

  HardwareProperties initial_hwprops = {
    0.000000,  // left edge
    0.000000,  // top edge
    95.934784,  // right edge
    65.259262,  // bottom edge
    1.000000,  // x pixels/TP width
    1.000000,  // y pixels/TP height
    25.400000,  // x screen DPI
    25.400000,  // y screen DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    2,  // max fingers
    5,  // max touch
    1,  // t5r2
    0,  // semi-mt
    1,   // is button pad
    0  // has_wheel
  };
  TestInterpreterWrapper wrapper(interpreter.get(), &initial_hwprops);

  // Actual data captured from Alex
  QuickSwipeTestInputs inputs[] = {
    // Two fingers arriving at the same time doing a swipe
    { 6.30188, 50.34, 53.29, 22.20, 91, 33.28, 56.05, 22.20, 92 },
    { 6.33170, 55.13, 32.40, 31.85, 91, 35.02, 42.20, 28.63, 92 },

    // Two fingers doing a swipe, but one arrives a few frames before
    // the other
    { 84.602478, 35.41, 65.27,  7.71, 93, -1.00, -1.00, -1.00, -1 },
    { 84.641174, 38.17, 43.07, 31.85, 93, -1.00, -1.00, -1.00, -1 },
    { 84.666290, 42.78, 21.66, 35.07, 93, 61.36, 31.83, 22.20, 94 },
    { 84.690422, 50.43,  5.37, 15.75, 93, 66.93, 12.64, 28.63, 94 },
  };

  base_interpreter = new LookaheadFilterInterpreterTestInterpreter;
  interpreter.reset(new LookaheadFilterInterpreter(
      NULL, base_interpreter, NULL));
  wrapper.Reset(interpreter.get());

  interpreter->min_delay_.val_ = 0.017;
  interpreter->max_delay_.val_ = 0.026;

  // Prime it w/ a dummy hardware state
  stime_t timeout = -1.0;
  HardwareState temp_hs = { 0.000001, 0, 0, 0, NULL, 0, 0, 0, 0 };
  wrapper.SyncInterpret(&temp_hs, &timeout);
  wrapper.HandleTimer(temp_hs.timestamp + timeout, NULL);

  std::set<short> input_ids;

  for (size_t i = 0; i < arraysize(inputs); i++) {
    const QuickSwipeTestInputs& in = inputs[i];
    FingerState fs[] = {
      // TM, Tm, WM, Wm, pr, orient, x, y, id, flags
      { 0, 0, 0, 0, in.pressure0, 0, in.x0, in.y0, in.id0, 0 },
      { 0, 0, 0, 0, in.pressure1, 0, in.x1, in.y1, in.id1, 0 },
    };
    unsigned short finger_cnt = in.id0 < 0 ? 0 : (in.id1 < 0 ? 1 : 2);
    HardwareState hs = { in.now, 0, finger_cnt, finger_cnt, fs, 0, 0, 0, 0 };

    for (size_t idx = 0; idx < finger_cnt; idx++)
      input_ids.insert(fs[idx].tracking_id);

    stime_t timeout = -1;
    wrapper.SyncInterpret(&hs, &timeout);
    if (timeout >= 0) {
      stime_t next_timestamp = INFINITY;
      if (i < arraysize(inputs) - 1)
        next_timestamp = inputs[i + 1].now;
      stime_t now = in.now;
      while (timeout >= 0 && (now + timeout) < next_timestamp) {
        now += timeout;
        timeout = -1;
        wrapper.HandleTimer(now, &timeout);
      }
    }
  }

  EXPECT_EQ(input_ids.size(), base_interpreter->all_ids_.size());
}

struct CyapaDrumrollTestInputs {
  bool reset_;
  stime_t now_;
  float x_, y_, pressure_;
  unsigned flags_;
  bool jump_here_;
};

// Replays a couple instances of drumroll from a Cyapa system as reported by
// Doug Anderson.
TEST(LookaheadFilterInterpreterTest, CyapaDrumrollTest) {
  LookaheadFilterInterpreterTestInterpreter* base_interpreter = NULL;
  std::unique_ptr<LookaheadFilterInterpreter> interpreter;

  HardwareProperties initial_hwprops = {
    0.000000,  // left edge
    0.000000,  // top edge
    106.666672,  // right edge
    68.000000,  // bottom edge
    1.000000,  // x pixels/TP width
    1.000000,  // y pixels/TP height
    25.400000,  // x screen DPI
    25.400000,  // y screen DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    15,  // max fingers
    5,  // max touch
    0,  // t5r2
    0,  // semi-mt
    0,  // is button pad
    0  // has_wheel
  };
  TestInterpreterWrapper wrapper(interpreter.get(), &initial_hwprops);

  CyapaDrumrollTestInputs inputs[] = {
    // First run:
    { true,  3.39245, 67.83, 34.10,  3.71, 0, false },
    { false, 3.40097, 67.83, 34.10, 30.88, 0, false },
    { false, 3.40970, 67.83, 34.10, 42.52, 0, false },
    { false, 3.44442, 67.83, 34.40, 44.46, 0, false },
    { false, 3.45307, 67.83, 34.60, 44.46, 0, false },
    { false, 3.46176, 67.83, 34.79, 44.46, 0, false },
    { false, 3.47041, 68.16, 35.20, 46.40, 0, false },
    { false, 3.47929, 68.83, 35.60, 50.28, 0, false },
    { false, 3.48806, 69.25, 35.90, 50.28, 0, false },
    { false, 3.49700, 69.75, 36.10, 52.22, 0, false },
    { false, 3.50590, 70.08, 36.50, 50.28, 0, false },
    { false, 3.51464, 70.41, 36.60, 52.22, 0, false },
    { false, 3.52355, 70.75, 36.79, 52.22, 0, false },
    { false, 3.53230, 71.00, 36.90, 52.22, 0, false },
    { false, 3.54118, 71.16, 36.90, 50.28, 0, false },
    { false, 3.55004, 71.33, 36.90, 52.22, 0, false },
    { false, 3.55900, 71.58, 37.10, 54.16, 0, false },
    { false, 3.56801, 71.66, 37.10, 52.22, 0, false },
    { false, 3.57690, 71.83, 37.10, 54.16, 0, false },
    { false, 3.59494, 71.91, 37.10, 52.22, 0, false },
    { false, 3.63092, 72.00, 37.10, 54.16, 0, false },
    { false, 3.68472, 72.00, 37.10, 56.10, 0, false },
    { false, 3.70275, 72.00, 37.10, 54.16, 0, false },
    { false, 3.71167, 72.00, 37.10, 52.22, 0, false },
    { false, 3.72067, 72.00, 37.10, 50.28, 0, false },
    { false, 3.72959, 72.00, 37.10, 42.52, 0, false },
    { false, 3.73863, 72.00, 37.10, 27.00, 0, false },
    // jump occurs here:
    { false, 3.74794, 14.58, 67.70, 17.30, 2, true },
    { false, 3.75703, 13.91, 66.59, 48.34, 2, false },
    { false, 3.76619, 13.91, 66.59, 50.28, 0, false },
    { false, 3.77531, 13.91, 66.20, 58.04, 0, false },
    { false, 3.78440, 13.91, 66.20, 58.04, 0, false },
    { false, 3.80272, 13.91, 66.00, 59.99, 0, false },
    { false, 3.81203, 13.91, 66.00, 61.93, 0, false },
    { false, 3.83017, 13.91, 66.00, 61.93, 0, false },
    { false, 3.83929, 13.91, 65.80, 61.93, 0, false },
    { false, 3.84849, 13.91, 65.70, 61.93, 0, false },
    { false, 3.90325, 13.91, 65.70, 59.99, 0, false },
    { false, 3.91247, 13.91, 65.70, 56.10, 0, false },
    { false, 3.92156, 13.91, 65.70, 42.52, 0, false },
    // Second run:
    { true,  39.25706, 91.08, 26.70, 25.06, 0, false },
    { false, 39.26551, 91.08, 26.70, 32.82, 0, false },
    { false, 39.27421, 89.66, 26.70, 34.76, 1, false },
    { false, 39.28285, 88.50, 26.70, 38.64, 1, false },
    { false, 39.29190, 87.75, 26.70, 42.52, 0, false },
    { false, 39.30075, 86.66, 26.70, 44.46, 0, false },
    { false, 39.30940, 85.66, 26.70, 44.46, 0, false },
    { false, 39.31812, 83.91, 26.70, 44.46, 0, false },
    { false, 39.32671, 82.25, 26.40, 48.34, 0, false },
    { false, 39.33564, 80.66, 26.30, 48.34, 0, false },
    { false, 39.34467, 79.25, 26.10, 50.28, 0, false },
    { false, 39.35335, 77.41, 26.00, 48.34, 0, false },
    { false, 39.36222, 75.50, 25.80, 52.22, 0, false },
    { false, 39.37135, 74.25, 25.80, 54.16, 0, false },
    { false, 39.38032, 73.25, 25.80, 56.10, 0, false },
    { false, 39.38921, 71.91, 25.80, 52.22, 0, false },
    { false, 39.39810, 70.25, 25.80, 56.10, 0, false },
    { false, 39.40708, 68.75, 25.80, 58.04, 0, false },
    { false, 39.41612, 67.75, 25.80, 59.99, 0, false },
    { false, 39.42528, 66.83, 25.70, 61.93, 0, false },
    { false, 39.43439, 66.25, 25.70, 58.04, 0, false },
    { false, 39.44308, 65.33, 25.70, 58.04, 0, false },
    { false, 39.45196, 64.41, 25.70, 59.99, 0, false },
    { false, 39.46083, 63.75, 25.70, 59.99, 0, false },
    { false, 39.46966, 63.25, 25.70, 61.93, 0, false },
    { false, 39.47855, 62.83, 25.70, 65.81, 0, false },
    { false, 39.48741, 62.50, 25.70, 63.87, 0, false },
    { false, 39.49632, 62.00, 25.70, 63.87, 0, false },
    { false, 39.50527, 61.66, 25.70, 61.93, 0, false },
    { false, 39.51422, 61.41, 25.70, 61.93, 0, false },
    { false, 39.52323, 61.08, 25.70, 63.87, 0, false },
    { false, 39.54126, 61.00, 25.70, 65.81, 0, false },
    { false, 39.55042, 60.83, 25.70, 65.81, 0, false },
    { false, 39.55951, 60.75, 25.70, 65.81, 0, false },
    { false, 39.56852, 60.33, 25.70, 63.87, 0, false },
    { false, 39.57756, 60.00, 25.70, 63.87, 0, false },
    { false, 39.58664, 59.58, 25.70, 63.87, 0, false },
    { false, 39.59553, 59.08, 25.70, 63.87, 0, false },
    { false, 39.60432, 58.50, 25.70, 67.75, 0, false },
    { false, 39.61319, 57.75, 25.70, 67.75, 0, false },
    { false, 39.62219, 57.33, 25.70, 67.75, 0, false },
    { false, 39.63088, 56.83, 25.70, 67.75, 0, false },
    { false, 39.63976, 56.33, 25.70, 67.75, 0, false },
    { false, 39.64860, 55.83, 25.70, 67.75, 0, false },
    { false, 39.65758, 55.33, 25.70, 65.81, 0, false },
    { false, 39.66647, 54.83, 25.70, 65.81, 0, false },
    { false, 39.67554, 54.33, 26.00, 69.69, 0, false },
    { false, 39.68430, 53.75, 26.10, 67.75, 0, false },
    { false, 39.69313, 53.33, 26.40, 67.75, 0, false },
    { false, 39.70200, 52.91, 26.40, 67.75, 0, false },
    { false, 39.71106, 52.33, 26.60, 67.75, 0, false },
    { false, 39.71953, 51.83, 26.80, 69.69, 0, false },
    { false, 39.72814, 51.41, 26.80, 71.63, 0, false },
    { false, 39.73681, 50.75, 26.80, 71.63, 0, false },
    { false, 39.74569, 50.33, 26.80, 71.63, 0, false },
    { false, 39.75422, 49.83, 26.80, 73.57, 0, false },
    { false, 39.76303, 49.33, 26.80, 75.51, 0, false },
    { false, 39.77166, 49.00, 26.80, 73.57, 0, false },
    { false, 39.78054, 48.41, 26.80, 75.51, 0, false },
    { false, 39.78950, 48.16, 26.80, 75.51, 0, false },
    { false, 39.79827, 47.83, 26.80, 75.51, 0, false },
    { false, 39.80708, 47.58, 26.80, 77.45, 0, false },
    { false, 39.81598, 47.41, 26.80, 77.45, 0, false },
    { false, 39.82469, 47.33, 26.80, 75.51, 0, false },
    { false, 39.83359, 47.25, 26.80, 75.51, 0, false },
    { false, 39.84252, 47.25, 26.80, 77.45, 0, false },
    { false, 39.87797, 47.25, 26.80, 75.51, 0, false },
    { false, 39.88674, 47.25, 26.80, 71.63, 0, false },
    { false, 39.89573, 47.25, 26.80, 67.75, 0, false },
    { false, 39.90444, 47.25, 26.80, 52.22, 0, false },
    { false, 39.91334, 47.25, 26.80, 27.00, 0, false },
    { false, 39.92267, 21.00, 62.29, 34.76, 1, true },
    { false, 39.93177, 21.00, 62.50, 46.40, 0, false },
    { false, 39.94127, 21.00, 62.79, 56.10, 2, false },
    { false, 39.95053, 21.00, 62.79, 61.93, 0, false },
    { false, 39.96006, 21.00, 62.79, 63.87, 0, false },
    { false, 39.96919, 21.00, 62.79, 65.81, 0, false },
    { false, 39.97865, 21.00, 62.79, 67.75, 0, false },
    { false, 39.99727, 21.00, 62.79, 67.75, 0, false },
    { false, 40.00660, 21.00, 62.79, 67.75, 0, false },
    { false, 40.02541, 21.00, 62.60, 67.75, 0, false },
    { false, 40.04417, 21.00, 62.60, 65.81, 0, false },
    { false, 40.05345, 21.00, 62.60, 63.87, 0, false },
    { false, 40.06241, 21.00, 62.60, 58.04, 0, false },
    { false, 40.07105, 21.00, 60.90, 34.76, 2, false },
    { false, 40.07990, 21.16, 59.10,  5.65, 2, false },
  };

  for (size_t i = 0; i < arraysize(inputs); i++) {
    const CyapaDrumrollTestInputs& input = inputs[i];
    FingerState fs = {
      // TM, Tm, WM, Wm, pr, orient, x, y, id, flags
      0, 0, 0, 0, input.pressure_, 0, input.x_, input.y_, 1, input.flags_
    };
    HardwareState hs = { input.now_, 0, 1, 1, &fs, 0, 0, 0, 0 };
    if (input.reset_) {
      if (base_interpreter) {
        EXPECT_TRUE(base_interpreter->expected_flags_at_occurred_);
      }
      base_interpreter = new LookaheadFilterInterpreterTestInterpreter;
      base_interpreter->expected_id_ = 1;
      interpreter.reset(new LookaheadFilterInterpreter(
          NULL, base_interpreter, NULL));
      wrapper.Reset(interpreter.get());
    }
    if (input.jump_here_) {
      base_interpreter->expected_flags_ =
          GESTURES_FINGER_WARP_X | GESTURES_FINGER_WARP_Y;
      base_interpreter->expected_flags_at_ = input.now_;
    }
    stime_t timeout = -1;
    wrapper.SyncInterpret(&hs, &timeout);
    if (timeout >= 0) {
      stime_t next_timestamp = INFINITY;
      if (i < arraysize(inputs) - 1)
        next_timestamp = inputs[i + 1].now_;
      stime_t now = input.now_;
      while (timeout >= 0 && (now + timeout) < next_timestamp) {
        now += timeout;
        timeout = -1;
        wrapper.HandleTimer(now, &timeout);
      }
    }
  }
  ASSERT_TRUE(base_interpreter);
  EXPECT_TRUE(base_interpreter->expected_flags_at_occurred_);
}

struct CyapaQuickTwoFingerMoveTestInputs {
  stime_t now;
  float x0, y0, pressure0;
  float x1, y1, pressure1;
  float x2, y2, pressure2;
};

// Using data from a real log, tests that when we do a swipe with large delay
// on Lumpy, we don't reassign finger IDs b/c we use sufficient temporary
// delay.
TEST(LookaheadFilterInterpreterTest, CyapaQuickTwoFingerMoveTest) {
  LookaheadFilterInterpreterTestInterpreter* base_interpreter =
      new LookaheadFilterInterpreterTestInterpreter;
  LookaheadFilterInterpreter interpreter(NULL, base_interpreter, NULL);
  interpreter.min_delay_.val_ = 0.0;

  HardwareProperties initial_hwprops = {
    0.000000,  // left edge
    0.000000,  // top edge
    106.666672,  // right edge
    68.000000,  // bottom edge
    1.000000,  // x pixels/TP width
    1.000000,  // y pixels/TP height
    25.400000,  // x screen DPI
    25.400000,  // y screen DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    15,  // max fingers
    5,  // max touch
    0,  // t5r2
    0,  // semi-mt
    0,  // is button pad
    0  // has_wheel
  };
  TestInterpreterWrapper wrapper(&interpreter, &initial_hwprops);

  CyapaQuickTwoFingerMoveTestInputs inputs[] = {
    { 1.13156, 38.16,  8.10, 52.2, 57.41,  6.40, 40.5, 75.66,  6.50, 36.7 },
    { 1.14369, 37.91, 17.50, 50.2, 56.83, 15.50, 40.5, 75.25, 15.30, 32.8 },
    { 1.15980, 34.66, 31.60, 48.3, 53.58, 29.40, 40.5, 72.25, 29.10, 28.9 }
  };
  // Prime it w/ a dummy hardware state
  stime_t timeout = -1.0;
  HardwareState temp_hs = { 0.000001, 0, 0, 0, NULL, 0, 0, 0, 0 };
  interpreter.SyncInterpret(&temp_hs, &timeout);

  base_interpreter->expected_ids_.insert(1);
  base_interpreter->expected_ids_.insert(2);
  base_interpreter->expected_ids_.insert(3);
  for (size_t i = 0; i < arraysize(inputs); i++) {
    const CyapaQuickTwoFingerMoveTestInputs& input = inputs[i];
    FingerState fs[] = {
      { 0, 0, 0, 0, input.pressure0, 0, input.x0, input.y0, 1, 0 },
      { 0, 0, 0, 0, input.pressure1, 0, input.x1, input.y1, 2, 0 },
      { 0, 0, 0, 0, input.pressure2, 0, input.x2, input.y2, 3, 0 }
    };
    HardwareState hs = { input.now,0,arraysize(fs),arraysize(fs),fs,0,0,0,0 };
    timeout = -1.0;
    interpreter.SyncInterpret(&hs, &timeout);
    if (timeout >= 0) {
      stime_t next_timestamp = INFINITY;
      if (i < arraysize(inputs) - 1)
        next_timestamp = inputs[i + 1].now;
      stime_t now = input.now;
      while (timeout >= 0 && (now + timeout) < next_timestamp) {
        now += timeout;
        timeout = -1;
        fprintf(stderr, "calling handler timer: %f\n", now);
        interpreter.HandleTimer(now, &timeout);
      }
    }
  }
}

TEST(LookaheadFilterInterpreterTest, SemiMtNoTrackingIdAssignmentTest) {
  LookaheadFilterInterpreterTestInterpreter* base_interpreter = NULL;
  std::unique_ptr<LookaheadFilterInterpreter> interpreter;

  HardwareProperties hwprops = {
    0, 0, 100, 100,  // left, top, right, bottom
    1,  // x res (pixels/mm)
    1,  // y res (pixels/mm)
    25, 25,  // scrn DPI X, Y
    -1,  // orientation minimum
    2,   // orientation maximum
    2, 5,  // max fingers, max_touch
    1, 1, 0, 0  // t5r2, semi, button pad
  };
  TestInterpreterWrapper wrapper(interpreter.get(), &hwprops);

  FingerState fs[] = {
    // TM, Tm, WM, Wm, pr, orient, x, y, id
    { 0, 0, 0, 0, 5, 0, 76, 45, 20, 0},  // 0

    { 0, 0, 0, 0, 62, 0, 56, 43, 20, 0},  // 1
    { 0, 0, 0, 0, 62, 0, 76, 41, 21, 0},

    { 0, 0, 0, 0, 76, 0, 56, 38, 20, 0},  // 3
    { 0, 0, 0, 0, 76, 0, 75, 35, 21, 0},

    { 0, 0, 0, 0, 78, 0, 56, 31, 20, 0},  // 5
    { 0, 0, 0, 0, 78, 0, 75, 27, 21, 0},

    { 0, 0, 0, 0, 78, 0, 56, 22, 20, 0},  // 7
    { 0, 0, 0, 0, 78, 0, 75, 18, 21, 0},

    // It will trigger the tracking id assignment for a quick move on a
    // non-semi-mt device.
    { 0, 0, 0, 0, 78, 0, 56, 13, 20, 0},  // 9
    { 0, 0, 0, 0, 78, 0, 75, 8, 21, 0}
  };

  HardwareState hs[] = {
    { 328.989039, 0, 1, 1, &fs[0], 0, 0, 0, 0 },
    { 329.013853, 0, 2, 2, &fs[1], 0, 0, 0, 0 },
    { 329.036266, 0, 2, 2, &fs[3], 0, 0, 0, 0 },
    { 329.061772, 0, 2, 2, &fs[5], 0, 0, 0, 0 },
    { 329.086734, 0, 2, 2, &fs[7], 0, 0, 0, 0 },
    { 329.110350, 0, 2, 2, &fs[9], 0, 0, 0, 0 },
  };

  base_interpreter = new LookaheadFilterInterpreterTestInterpreter;
  interpreter.reset(new LookaheadFilterInterpreter(
      NULL, base_interpreter, NULL));
  wrapper.Reset(interpreter.get());

  stime_t timeout = -1.0;
  List<LookaheadFilterInterpreter::QState>* queue = &interpreter->queue_;

  wrapper.SyncInterpret(&hs[0], &timeout);
  EXPECT_EQ(queue->Tail()->fs_[0].tracking_id, 20);

  // Test if the fingers in queue have the same tracking ids from input.
  for (size_t i = 1; i < arraysize(hs); i++) {
    wrapper.SyncInterpret(&hs[i], &timeout);
    EXPECT_EQ(queue->Tail()->fs_[0].tracking_id, 20);  // the same input id
    EXPECT_EQ(queue->Tail()->fs_[1].tracking_id, 21);
  }
}
}  // namespace gestures
