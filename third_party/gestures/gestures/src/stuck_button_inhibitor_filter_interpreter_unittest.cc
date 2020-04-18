// Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <deque>
#include <math.h>
#include <vector>
#include <utility>

#include <gtest/gtest.h>

#include "gestures/include/gestures.h"
#include "gestures/include/stuck_button_inhibitor_filter_interpreter.h"
#include "gestures/include/unittest_util.h"
#include "gestures/include/util.h"

using std::deque;
using std::make_pair;
using std::pair;
using std::vector;

namespace gestures {

class StuckButtonInhibitorFilterInterpreterTest : public ::testing::Test {};

class StuckButtonInhibitorFilterInterpreterTestInterpreter :
      public Interpreter {
 public:
  StuckButtonInhibitorFilterInterpreterTestInterpreter()
      : Interpreter(NULL, NULL, false),
        called_(false) {}

  virtual void SyncInterpret(HardwareState* hwstate, stime_t* timeout) {
    HandleTimer(0.0, timeout);
  }

  virtual void HandleTimer(stime_t now, stime_t* timeout) {
    called_ = true;
    if (return_values_.empty())
      return;
    pair<Gesture, stime_t> val = return_values_.front();
    return_values_.pop_front();
    if (val.second >= 0.0)
      *timeout = val.second;
    return_value_ = val.first;
    if (return_value_.type == kGestureTypeNull)
      return;
    ProduceGesture(return_value_);
  }

  bool called_;
  Gesture return_value_;
  deque<pair<Gesture, stime_t> > return_values_;
};

namespace {
struct Record {
  stime_t now_;  // if >= 0, call HandleTimeout, else call SyncInterpret
  HardwareState hs_;
  bool should_call_next_;
  stime_t expected_timeout_;  // out
  Gesture expected_gs_;  // out
  stime_t next_timeout_;
  Gesture next_gs_;
};

// Compares gestures, but doesn't include timestamps in the comparison
bool GestureEq(const Gesture& a, const Gesture& b) {
  Gesture a_copy = a;
  Gesture b_copy = b;
  a_copy.start_time = a_copy.end_time = b_copy.start_time = b_copy.end_time = 0;
  return a_copy == b_copy;
}
}  // namespace {}

TEST(StuckButtonInhibitorFilterInterpreterTest, SimpleTest) {
  StuckButtonInhibitorFilterInterpreterTestInterpreter* base_interpreter =
      new StuckButtonInhibitorFilterInterpreterTestInterpreter;
  StuckButtonInhibitorFilterInterpreter interpreter(base_interpreter, NULL);

  HardwareProperties hwprops = {
    0, 0, 100, 100,  // left, top, right, bottom
    10,  // x res (pixels/mm)
    10,  // y res (pixels/mm)
    133, 133,  // scrn DPI X, Y
    -1,  // orientation minimum
    2,   // orientation maximum
    2, 5,  // max fingers, max_touch
    0, 0, 0, 0  //t5r2, semi, button pad
  };
  TestInterpreterWrapper wrapper(&interpreter, &hwprops);

  Gesture null;
  Gesture move = Gesture(kGestureMove,
                         0,  // start time
                         0,  // end time
                         -4,  // dx
                         2.8);  // dy
  Gesture down = Gesture(kGestureButtonsChange,
                         0,  // start time
                         0,  // end time
                         GESTURES_BUTTON_LEFT,  // down
                         0);  // up
  Gesture up = Gesture(kGestureButtonsChange,
                       0,  // start time
                       0,  // end time
                       0,  // down
                       GESTURES_BUTTON_LEFT);  // up
  Gesture rdwn = Gesture(kGestureButtonsChange,
                         0,  // start time
                         0,  // end time
                         GESTURES_BUTTON_RIGHT,  // down
                         0);  // up
  Gesture rup = Gesture(kGestureButtonsChange,
                        0,  // start time
                        0,  // end time
                        0,  // down
                        GESTURES_BUTTON_RIGHT);  // up
  Gesture rldn = Gesture(kGestureButtonsChange,
                         0,  // start time
                         0,  // end time
                         GESTURES_BUTTON_LEFT | GESTURES_BUTTON_RIGHT,  // down
                         0);  // up
  Gesture rlup = Gesture(kGestureButtonsChange,
                         0,  // start time
                         0,  // end time
                         0,  // down
                         GESTURES_BUTTON_LEFT | GESTURES_BUTTON_RIGHT);  // up

  FingerState fs = { 0, 0, 0, 0, 1, 0, 150, 4000, 1, 0 };
  Record recs[] = {
    // Simple move. Nothing button related
    { -1.0, { 1.0, 0, 1, 1, &fs, 0, 0, 0, 0  }, true,  -1.0, null, -1.0, null },
    { -1.0, { 1.1, 0, 1, 1, &fs, 0, 0, 0, 0  }, true,  -1.0, move, -1.0, move },
    // Button down, followed by nothing, so we timeout and send button up
    { -1.0, { 1.2, 0, 1, 1, &fs, 0, 0, 0, 0  }, true,  -1.0, down, -1.0, down },
    { -1.0, { 1.3, 0, 0, 0, NULL, 0, 0, 0, 0 }, true,   1.0, null, -1.0, null },
    {  2.3, { 0.0, 0, 0, 0, NULL, 0, 0, 0, 0 }, false, -1.0, up,   -1.0, null },
    // Next sends button up in timeout
    { -1.0, { 3.2, 0, 1, 1, &fs, 0, 0, 0, 0  }, true,  -1.0, down, -1.0, down },
    { -1.0, { 3.3, 0, 0, 0, NULL, 0, 0, 0, 0 }, true,   0.5, null,  0.5, null },
    {  3.8, { 0.0, 0, 0, 0, NULL, 0, 0, 0, 0 }, true,  -1.0, up,   -1.0, up   },
    // Double down/up squash
    { -1.0, { 4.2, 0, 1, 1, &fs, 0, 0, 0, 0  }, true,  -1.0, down, -1.0, down },
    { -1.0, { 4.3, 0, 1, 1, &fs, 0, 0, 0, 0  }, true,  -1.0, null, -1.0, down },
    { -1.0, { 4.4, 0, 0, 0, NULL, 0, 0, 0, 0 }, true,  -1.0, up,   -1.0, up   },
    { -1.0, { 4.5, 0, 0, 0, NULL, 0, 0, 0, 0 }, true,  -1.0, null, -1.0, up   },
    // Right down, left double up/down squash
    { -1.0, { 5.1, 0, 1, 1, &fs, 0, 0, 0, 0  }, true,  -1.0, rdwn, -1.0, rdwn },
    { -1.0, { 5.2, 0, 1, 1, &fs, 0, 0, 0, 0  }, true,  -1.0, down, -1.0, rldn },
    { -1.0, { 5.3, 0, 1, 1, &fs, 0, 0, 0, 0  }, true,  -1.0, null, -1.0, down },
    { -1.0, { 5.4, 0, 0, 0, NULL, 0, 0, 0, 0 }, true,   1.0, rup,  -1.0, rup  },
    { -1.0, { 5.5, 0, 0, 0, NULL, 0, 0, 0, 0 }, true,  -1.0, up,   -1.0, rlup },
  };

  for (size_t i = 0; i < arraysize(recs); ++i) {
    Record& rec = recs[i];
    base_interpreter->return_values_.clear();
    if (rec.should_call_next_) {
      base_interpreter->return_values_.push_back(
          make_pair(rec.next_gs_, rec.next_timeout_));
    }
    stime_t timeout = -1.0;
    Gesture* result = NULL;
    if (rec.now_ < 0.0) {
      result = wrapper.SyncInterpret(&rec.hs_, &timeout);
    } else {
      result = wrapper.HandleTimer(rec.now_, &timeout);
    }
    if (!result)
      result = &null;
    EXPECT_TRUE(GestureEq(*result, rec.expected_gs_)) << "i=" << i;
    EXPECT_DOUBLE_EQ(timeout, rec.expected_timeout_) << "i=" << i;
  }
}

}  // namespace gestures
