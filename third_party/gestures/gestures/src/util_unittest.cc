// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>

#include "gestures/include/macros.h"
#include "gestures/include/util.h"

namespace gestures {

class UtilTest : public ::testing::Test {};

TEST(UtilTest, DistSqTest) {
  FingerState fs[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID
    {0, 0, 0, 0, 1, 0, 1, 2, 1, 0},
    {0, 0, 0, 0, 1, 0, 4, 6, 1, 0}
  };
  EXPECT_FLOAT_EQ(DistSq(fs[0], fs[1]), 25);
  EXPECT_FLOAT_EQ(DistSqXY(fs[0], 4, 6), 25);
}

namespace {
struct GesturesRec {
  Gesture gs_;
  Gesture addend_;
  Gesture expected_;
};
}  // namespace {}

TEST(UtilTest, CombineGesturesTest) {
  Gesture null;
  Gesture move = Gesture(kGestureMove,
                         0,  // start time
                         0,  // end time
                         -4,  // dx
                         2.8);  // dy
  Gesture dbl_move = Gesture(kGestureMove,
                             0,  // start time
                             0,  // end time
                             -8,  // dx
                             5.6);  // dy
  Gesture scroll = Gesture(kGestureScroll,
                           0,  // start time
                           0,  // end time
                           -4,  // dx
                           2.8);  // dy
  Gesture dbl_scroll = Gesture(kGestureScroll,
                               0,  // start time
                               0,  // end time
                               -8,  // dx
                               5.6);  // dy
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
  Gesture click = Gesture(kGestureButtonsChange,
                          0,  // start time
                          0,  // end time
                          GESTURES_BUTTON_LEFT,  // down
                          GESTURES_BUTTON_LEFT);  // up
  Gesture rdown = Gesture(kGestureButtonsChange,
                          0,  // start time
                          0,  // end time
                          GESTURES_BUTTON_RIGHT,  // down
                          0);  // up
  Gesture rup = Gesture(kGestureButtonsChange,
                        0,  // start time
                        0,  // end time
                        0,  // down
                        GESTURES_BUTTON_RIGHT);  // up
  Gesture rclick = Gesture(kGestureButtonsChange,
                           0,  // start time
                           0,  // end time
                           GESTURES_BUTTON_RIGHT,  // down
                           GESTURES_BUTTON_RIGHT);  // up
  Gesture tapdown = Gesture(kGestureFling,
                            0,  // start time
                            0,  // end time
                            0,  // vx
                            0,  // vy
                            GESTURES_FLING_TAP_DOWN);  // flags

  GesturesRec recs[] = {
    { null, null, null },
    { null, move, move },
    { null, scroll, scroll },
    { move, null, move },
    { scroll, null, scroll },
    { move, scroll, scroll },
    { scroll, move, move },
    { move, move, dbl_move },
    { scroll, scroll, dbl_scroll },
    { move, down, down },
    { scroll, up, up },
    { rup, move, rup },
    { rdown, scroll, rdown },
    { null, click, click },
    { click, null, click },
    { tapdown, move, tapdown },
    { move, tapdown, tapdown },
    // button only tests:
    { up, down, null },  // the special case
    { up, click, up },
    { down, up, click },
    { click, down, down },
    { click, click, click },
    // with right button:
    { rup, rdown, null },  // the special case
    { rup, rclick, rup },
    { rdown, rup, rclick },
    { rclick, rdown, rdown },
    { rclick, rclick, rclick }
  };
  for (size_t i = 0; i < arraysize(recs); ++i) {
    Gesture gs = recs[i].gs_;
    gestures::CombineGestures(
        &gs,
        recs[i].addend_.type == kGestureTypeNull ? NULL : &recs[i].addend_);
    EXPECT_TRUE(gs == recs[i].expected_) << "i=" << i;
  }
}
}  // namespace gestures
