// Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <vector>

#include <gtest/gtest.h>

#include "gestures/include/gestures.h"
#include "gestures/include/immediate_interpreter.h"
#include "gestures/include/string_util.h"
#include "gestures/include/unittest_util.h"
#include "gestures/include/util.h"

namespace gestures {

using std::string;

class ImmediateInterpreterTest : public ::testing::Test {};

TEST(ImmediateInterpreterTest, MoveDownTest) {
  ImmediateInterpreter ii(NULL, NULL);

  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    1000,  // right edge
    1000,  // bottom edge
    500,  // pixels/TP width
    500,  // pixels/TP height
    96,  // screen DPI x
    96,  // screen DPI y
    -1,  // orientation minimum
    2,   // orientation maximum
    2,  // max fingers
    5,  // max touch
    0,  // tripletap
    0,  // semi-mt
    1,  // is button pad
    0   // has_wheel
  };
  TestInterpreterWrapper wrapper(&ii, &hwprops);

  FingerState finger_states[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID
    {0, 0, 0, 0, 1, 0, 10, 10, 1, 0},
    {0, 0, 0, 0, 1, 0, 10, 20, 1, 0},
    {0, 0, 0, 0, 1, 0, 20, 20, 1, 0}
  };
  HardwareState hardware_states[] = {
    // time, buttons down, finger count, finger states pointer
    { 200000, 0, 1, 1, &finger_states[0], 0, 0, 0, 0 },
    { 210000, 0, 1, 1, &finger_states[1], 0, 0, 0, 0 },
    { 220000, 0, 1, 1, &finger_states[2], 0, 0, 0, 0 },
    { 230000, 0, 0, 0, NULL, 0, 0, 0, 0 },
    { 240000, 0, 0, 0, NULL, 0, 0, 0, 0 }
  };

  EXPECT_EQ(NULL, wrapper.SyncInterpret(&hardware_states[0], NULL));

  Gesture* gs = wrapper.SyncInterpret(&hardware_states[1], NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), gs);
  EXPECT_EQ(kGestureTypeMove, gs->type);
  EXPECT_EQ(0, gs->details.move.dx);
  EXPECT_EQ(10, gs->details.move.dy);
  EXPECT_EQ(200000, gs->start_time);
  EXPECT_EQ(210000, gs->end_time);

  gs = wrapper.SyncInterpret(&hardware_states[2], NULL);
  EXPECT_NE(reinterpret_cast<Gesture*>(NULL), gs);
  EXPECT_EQ(kGestureTypeMove, gs->type);
  EXPECT_EQ(10, gs->details.move.dx);
  EXPECT_EQ(0, gs->details.move.dy);
  EXPECT_EQ(210000, gs->start_time);
  EXPECT_EQ(220000, gs->end_time);

  EXPECT_EQ(reinterpret_cast<Gesture*>(NULL),
            wrapper.SyncInterpret(&hardware_states[3], NULL));
  EXPECT_EQ(reinterpret_cast<Gesture*>(NULL),
            wrapper.SyncInterpret(&hardware_states[4], NULL));
}

TEST(ImmediateInterpreterTest, MoveUpWithRestingThumbTest) {
  ImmediateInterpreter ii(NULL, NULL);

  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    1000,  // right edge
    1000,  // bottom edge
    50,  // pixels/TP width
    50,  // pixels/TP height
    96,  // screen DPI x
    96,  // screen DPI y
    -1,  // orientation minimum
    2,   // orientation maximum
    2,  // max fingers
    5,  // max touch
    0,  // tripletap
    0,  // semi-mt
    1,  // is button pad
    0   // has_wheel
  };
  TestInterpreterWrapper wrapper(&ii, &hwprops);

  FingerState finger_states[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID
    {0, 0, 0, 0, 10, 0, 500, 999, 1, 0},
    {0, 0, 0, 0, 10, 0, 500, 950, 2, 0},
    {0, 0, 0, 0, 10, 0, 500, 999, 1, 0},
    {0, 0, 0, 0, 10, 0, 500, 940, 2, 0},
    {0, 0, 0, 0, 10, 0, 500, 999, 1, 0},
    {0, 0, 0, 0, 10, 0, 500, 930, 2, 0}
  };
  HardwareState hardware_states[] = {
    // time, buttons down, finger count, finger states pointer
    { 200000, 0, 2, 2, &finger_states[0], 0, 0, 0, 0 },
    { 210000, 0, 2, 2, &finger_states[2], 0, 0, 0, 0 },
    { 220000, 0, 2, 2, &finger_states[4], 0, 0, 0, 0 },
    { 230000, 0, 0, 0, NULL, 0, 0, 0, 0 },
    { 240000, 0, 0, 0, NULL, 0, 0, 0, 0 }
  };

  EXPECT_EQ(NULL, wrapper.SyncInterpret(&hardware_states[0], NULL));

  Gesture* gs = wrapper.SyncInterpret(&hardware_states[1], NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), gs);
  EXPECT_EQ(kGestureTypeMove, gs->type);
  EXPECT_EQ(0, gs->details.move.dx);
  EXPECT_EQ(-10, gs->details.move.dy);
  EXPECT_EQ(200000, gs->start_time);
  EXPECT_EQ(210000, gs->end_time);

  gs = wrapper.SyncInterpret(&hardware_states[2], NULL);
  EXPECT_NE(reinterpret_cast<Gesture*>(NULL), gs);
  EXPECT_EQ(kGestureTypeMove, gs->type);
  EXPECT_EQ(0, gs->details.move.dx);
  EXPECT_EQ(-10, gs->details.move.dy);
  EXPECT_EQ(210000, gs->start_time);
  EXPECT_EQ(220000, gs->end_time);

  EXPECT_EQ(reinterpret_cast<Gesture*>(NULL),
            wrapper.SyncInterpret(&hardware_states[3], NULL));
  EXPECT_EQ(reinterpret_cast<Gesture*>(NULL),
            wrapper.SyncInterpret(&hardware_states[4], NULL));
}

TEST(ImmediateInterpreterTest, SemiMtScrollUpWithRestingThumbTest) {
  ImmediateInterpreter ii(NULL, NULL);
  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    1000,  // right edge
    1000,  // bottom edge
    20,  // pixels/TP width
    20,  // pixels/TP height
    96,  // x screen DPI
    96,  // y screen DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    2,  // max fingers
    3,  // max touch
    0,  // tripletap
    1,  // semi-mt
    1,  // is button pad
    0   // has_wheel
  };
  TestInterpreterWrapper wrapper(&ii, &hwprops);

  FingerState finger_states[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID
    {0, 0, 0, 0, 100, 0, 50, 950, 1, 0},
    {0, 0, 0, 0, 100, 0, 415, 900, 2, 0},

    {0, 0, 0, 0, 100, 0, 50, 950, 1, 0},
    {0, 0, 0, 0, 100, 0, 415, 800, 2, 0},

    {0, 0, 0, 0, 100, 0, 50, 950, 1, 0},
    {0, 0, 0, 0, 100, 0, 415, 700, 2, 0},
  };
  HardwareState hardware_states[] = {
    // time, buttons, finger count, touch count, finger states pointer
    { 0.200000, 0, 2, 3, &finger_states[0], 0, 0, 0, 0 },
    { 0.250000, 0, 2, 3, &finger_states[2], 0, 0, 0, 0 },
    { 0.300000, 0, 2, 3, &finger_states[4], 0, 0, 0, 0 }
  };

  EXPECT_EQ(NULL, wrapper.SyncInterpret(&hardware_states[0], NULL));

  Gesture* gs = wrapper.SyncInterpret(&hardware_states[1], NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), gs);
  EXPECT_EQ(kGestureTypeScroll, gs->type);
  EXPECT_FLOAT_EQ(0, gs->details.move.dx);
  EXPECT_FLOAT_EQ(-100, gs->details.move.dy);
  EXPECT_DOUBLE_EQ(0.200000, gs->start_time);
  EXPECT_DOUBLE_EQ(0.250000, gs->end_time);

  gs = wrapper.SyncInterpret(&hardware_states[2], NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), gs);
  EXPECT_EQ(kGestureTypeScroll, gs->type);
  EXPECT_FLOAT_EQ(0, gs->details.move.dx);
  EXPECT_FLOAT_EQ(-100, gs->details.move.dy);
  EXPECT_DOUBLE_EQ(0.250000, gs->start_time);
  EXPECT_DOUBLE_EQ(0.300000, gs->end_time);
}

void ScrollUpTest(float pressure_a, float pressure_b) {
  ImmediateInterpreter ii(NULL, NULL);
  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    1000,  // right edge
    1000,  // bottom edge
    20,  // pixels/TP width
    20,  // pixels/TP height
    96,  // x screen DPI
    96,  // y screen DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    2,  // max fingers
    5,  // max touch
    0,  // tripletap
    0,  // semi-mt
    1,  // is button pad
    0   // has_wheel
  };
  TestInterpreterWrapper wrapper(&ii, &hwprops);

  float p_a = pressure_a;
  float p_b = pressure_b;

  FingerState finger_states[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID
    {0, 0, 0, 0, p_a, 0, 400, 900, 1, 0},
    {0, 0, 0, 0, p_b, 0, 415, 900, 2, 0},

    {0, 0, 0, 0, p_a, 0, 400, 800, 1, 0},
    {0, 0, 0, 0, p_b, 0, 415, 800, 2, 0},

    {0, 0, 0, 0, p_a, 0, 400, 700, 1, 0},
    {0, 0, 0, 0, p_b, 0, 415, 700, 2, 0},
  };
  HardwareState hardware_states[] = {
    // time, buttons, finger count, touch count, finger states pointer
    { 0.200000, 0, 2, 2, &finger_states[0], 0, 0, 0, 0 },
    { 0.250000, 0, 2, 2, &finger_states[2], 0, 0, 0, 0 },
    { 0.300000, 0, 2, 2, &finger_states[4], 0, 0, 0, 0 }
  };

  EXPECT_EQ(NULL, wrapper.SyncInterpret(&hardware_states[0], NULL));

  Gesture* gs = wrapper.SyncInterpret(&hardware_states[1], NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), gs);
  EXPECT_EQ(kGestureTypeScroll, gs->type);
  EXPECT_FLOAT_EQ(0, gs->details.move.dx);
  EXPECT_FLOAT_EQ(-100, gs->details.move.dy);
  EXPECT_DOUBLE_EQ(0.200000, gs->start_time);
  EXPECT_DOUBLE_EQ(0.250000, gs->end_time);

  gs = wrapper.SyncInterpret(&hardware_states[2], NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), gs);
  EXPECT_EQ(kGestureTypeScroll, gs->type);
  EXPECT_FLOAT_EQ(0, gs->details.move.dx);
  EXPECT_FLOAT_EQ(-100, gs->details.move.dy);
  EXPECT_DOUBLE_EQ(0.250000, gs->start_time);
  EXPECT_DOUBLE_EQ(0.300000, gs->end_time);
}

TEST(ImmediateInterpreterTest, ScrollUpTest) {
  ScrollUpTest(24, 92);
}

TEST(ImmediateInterpreterTest, FatFingerScrollUpTest) {
  ScrollUpTest(125, 185);
}

// Tests that a tap immediately after a scroll doesn't generate a click.
// Such a tap would be unrealistic to come from a human.
TEST(ImmediateInterpreterTest, ScrollThenFalseTapTest) {
  ImmediateInterpreter ii(NULL, NULL);
  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    1000,  // right edge
    1000,  // bottom edge
    20,  // pixels/TP width
    20,  // pixels/TP height
    96,  // x screen DPI
    96,  // y screen DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    2,  // max fingers
    5,  // max touch
    0,  // tripletap
    0,  // semi-mt
    1,  // is button pad
    0   // has_wheel
  };
  TestInterpreterWrapper wrapper(&ii, &hwprops);

  FingerState finger_states[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID
    {0, 0, 0, 0, 20, 0, 400, 900, 1, 0},
    {0, 0, 0, 0, 20, 0, 415, 900, 2, 0},

    {0, 0, 0, 0, 20, 0, 400, 800, 1, 0},
    {0, 0, 0, 0, 20, 0, 415, 800, 2, 0},

    {0, 0, 0, 0, 20, 0, 400, 700, 1, 0},
    {0, 0, 0, 0, 20, 0, 415, 700, 2, 0},

    {0, 0, 0, 0, 20, 0, 400, 600, 3, 0},
  };
  HardwareState hardware_states[] = {
    // time, buttons, finger count, touch count, finger states pointer
    { 0.200000, 0, 2, 2, &finger_states[0], 0, 0, 0, 0 },
    { 0.250000, 0, 2, 2, &finger_states[2], 0, 0, 0, 0 },
    { 0.300000, 0, 2, 2, &finger_states[4], 0, 0, 0, 0 },
    { 0.310000, 0, 0, 0, NULL, 0, 0, 0, 0 },
    { 0.320000, 0, 1, 1, &finger_states[6], 0, 0, 0, 0 },
    { 0.330000, 0, 0, 0, NULL, 0, 0, 0, 0 }
  };

  ii.tap_enable_.val_ = 1;
  EXPECT_EQ(NULL, wrapper.SyncInterpret(&hardware_states[0], NULL));

  Gesture* gs = wrapper.SyncInterpret(&hardware_states[1], NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), gs);
  EXPECT_EQ(kGestureTypeScroll, gs->type);
  gs = wrapper.SyncInterpret(&hardware_states[2], NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), gs);
  EXPECT_EQ(kGestureTypeScroll, gs->type);
  gs = wrapper.SyncInterpret(&hardware_states[3], NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), gs);
  EXPECT_EQ(kGestureTypeFling, gs->type);
  gs = wrapper.SyncInterpret(&hardware_states[4], NULL);
  ASSERT_EQ(reinterpret_cast<Gesture*>(NULL), gs);
  stime_t timeout = -1.0;
  gs = wrapper.SyncInterpret(&hardware_states[5], &timeout);
  ASSERT_EQ(reinterpret_cast<Gesture*>(NULL), gs);
  // If it were a tap, timeout would be > 0, but this shouldn't be a tap,
  // so timeout should be negative still.
  EXPECT_LT(timeout, 0.0);
}

// Tests that a consistent scroll has predictable fling, and that increasing
// scrolls have a fling as least as fast the second to last scroll.
TEST(ImmediateInterpreterTest, FlingTest) {
  ImmediateInterpreter ii(NULL, NULL);
  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    100,  // right edge
    100,  // bottom edge
    1,  // pixels/TP width
    1,  // pixels/TP height
    96,  // x screen DPI
    96,  // y screen DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    2,  // max fingers
    5,  // max touch
    0,  // tripletap
    0,  // semi-mt
    1,  // is button pad
    0   // has_wheel
  };
  TestInterpreterWrapper wrapper(&ii, &hwprops);

  FingerState finger_states[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID
    // Consistent movement for 4 frames
    {0, 0, 0, 0, 20, 0, 40, 20, 1, 0},
    {0, 0, 0, 0, 20, 0, 60, 20, 2, 0},

    {0, 0, 0, 0, 20, 0, 40, 30, 1, 0},
    {0, 0, 0, 0, 20, 0, 60, 30, 2, 0},

    {0, 0, 0, 0, 20, 0, 40, 40, 1, 0},
    {0, 0, 0, 0, 20, 0, 60, 40, 2, 0},

    {0, 0, 0, 0, 20, 0, 40, 50, 1, 0},
    {0, 0, 0, 0, 20, 0, 60, 50, 2, 0},

    // Increasing movement for 4 frames
    {0, 0, 0, 0, 20, 0, 40, 20, 3, 0},
    {0, 0, 0, 0, 20, 0, 60, 20, 4, 0},

    {0, 0, 0, 0, 20, 0, 40, 25, 3, 0},
    {0, 0, 0, 0, 20, 0, 60, 25, 4, 0},

    {0, 0, 0, 0, 20, 0, 40, 35, 3, 0},
    {0, 0, 0, 0, 20, 0, 60, 35, 4, 0},

    {0, 0, 0, 0, 20, 0, 40, 50, 3, 0},
    {0, 0, 0, 0, 20, 0, 60, 50, 4, 0},
  };
  HardwareState hardware_states[] = {
    // time, buttons, finger count, touch count, finger states pointer
    { 0.00, 0, 2, 2, &finger_states[0], 0, 0, 0, 0 },
    { 1.00, 0, 2, 2, &finger_states[0], 0, 0, 0, 0 },
    { 1.01, 0, 2, 2, &finger_states[2], 0, 0, 0, 0 },
    { 1.02, 0, 2, 2, &finger_states[4], 0, 0, 0, 0 },
    { 1.03, 0, 2, 2, &finger_states[6], 0, 0, 0, 0 },
    { 1.04, 0, 0, 0, NULL, 0, 0, 0, 0 },

    { 3.00, 0, 2, 2, &finger_states[8], 0, 0, 0, 0 },
    { 4.00, 0, 2, 2, &finger_states[8], 0, 0, 0, 0 },
    { 4.01, 0, 2, 2, &finger_states[10], 0, 0, 0, 0 },
    { 4.02, 0, 2, 2, &finger_states[12], 0, 0, 0, 0 },
    { 4.03, 0, 2, 2, &finger_states[14], 0, 0, 0, 0 },
    { 4.04, 0, 0, 0, NULL, 0, 0, 0, 0 },
  };

  size_t idx = 0;

  // Consistent movement
  EXPECT_EQ(NULL, wrapper.SyncInterpret(&hardware_states[idx++], NULL));
  EXPECT_EQ(NULL, wrapper.SyncInterpret(&hardware_states[idx++], NULL));

  Gesture* gs = wrapper.SyncInterpret(&hardware_states[idx++], NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), gs);
  EXPECT_EQ(kGestureTypeScroll, gs->type);
  gs = wrapper.SyncInterpret(&hardware_states[idx++], NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), gs);
  EXPECT_EQ(kGestureTypeScroll, gs->type);
  gs = wrapper.SyncInterpret(&hardware_states[idx++], NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), gs);
  EXPECT_EQ(kGestureTypeScroll, gs->type);
  gs = wrapper.SyncInterpret(&hardware_states[idx++], NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), gs);
  EXPECT_EQ(kGestureTypeFling, gs->type);
  EXPECT_FLOAT_EQ(0, gs->details.fling.vx);
  EXPECT_FLOAT_EQ(10 / 0.01, gs->details.fling.vy);

  // Increasing speed movement
  gs = wrapper.SyncInterpret(&hardware_states[idx++], NULL);
  EXPECT_EQ(reinterpret_cast<Gesture*>(NULL), gs) << gs->String();
  gs = wrapper.SyncInterpret(&hardware_states[idx++], NULL);
  EXPECT_EQ(reinterpret_cast<Gesture*>(NULL), gs) << gs->String();

  gs = wrapper.SyncInterpret(&hardware_states[idx++], NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), gs);
  EXPECT_EQ(kGestureTypeScroll, gs->type);
  gs = wrapper.SyncInterpret(&hardware_states[idx++], NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), gs);
  EXPECT_EQ(kGestureTypeScroll, gs->type);
  gs = wrapper.SyncInterpret(&hardware_states[idx++], NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), gs);
  EXPECT_EQ(kGestureTypeScroll, gs->type);
  gs = wrapper.SyncInterpret(&hardware_states[idx++], NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), gs);
  EXPECT_EQ(kGestureTypeFling, gs->type);
  EXPECT_FLOAT_EQ(0, gs->details.fling.vx);
  EXPECT_FLOAT_EQ(1250, gs->details.fling.vy);
}

// Tests that fingers that have been present a while, but are stationary,
// can be evaluated multiple times when they start moving.
TEST(ImmediateInterpreterTest, DelayedStartScrollTest) {
  ImmediateInterpreter ii(NULL, NULL);
  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    100,  // right edge
    100,  // bottom edge
    1,  // pixels/TP width
    1,  // pixels/TP height
    96,  // x screen DPI
    96,  // y screen DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    2,  // max fingers
    5,  // max touch
    0,  // tripletap
    0,  // semi-mt
    1,  // is button pad
    0   // has_wheel
  };
  TestInterpreterWrapper wrapper(&ii, &hwprops);

  FingerState finger_states[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID
    // Consistent movement for 4 frames
    {0, 0, 0, 0, 20, 0, 40, 95, 1, 0},
    {0, 0, 0, 0, 20, 0, 60, 95, 2, 0},

    {0, 0, 0, 0, 20, 0, 40, 95, 1, 0},
    {0, 0, 0, 0, 20, 0, 60, 85, 2, 0},

    {0, 0, 0, 0, 20, 0, 40, 80, 1, 0},
    {0, 0, 0, 0, 20, 0, 60, 75, 2, 0},
  };
  HardwareState hardware_states[] = {
    // time, buttons, finger count, touch count, finger states pointer
    { 1.00, 0, 2, 2, &finger_states[0], 0, 0, 0, 0 },
    { 2.00, 0, 2, 2, &finger_states[0], 0, 0, 0, 0 },
    { 2.01, 0, 2, 2, &finger_states[2], 0, 0, 0, 0 },
    { 2.02, 0, 2, 2, &finger_states[4], 0, 0, 0, 0 },
    { 2.03, 0, 0, 0, NULL, 0, 0, 0, 0 },
  };

  size_t idx = 0;

  // Consistent movement
  EXPECT_EQ(NULL, wrapper.SyncInterpret(&hardware_states[idx++], NULL));
  EXPECT_EQ(NULL, wrapper.SyncInterpret(&hardware_states[idx++], NULL));

  Gesture* gs = wrapper.SyncInterpret(&hardware_states[idx++], NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), gs);
  EXPECT_EQ(kGestureTypeMove, gs->type);

  gs = wrapper.SyncInterpret(&hardware_states[idx++], NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), gs);
  EXPECT_EQ(kGestureTypeScroll, gs->type);
}

// Tests that after a scroll is happening, if a finger lets go, scrolling stops.
TEST(ImmediateInterpreterTest, ScrollReevaluateTest) {
  ImmediateInterpreter ii(NULL, NULL);
  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    100,  // right edge
    100,  // bottom edge
    1,  // pixels/TP width
    1,  // pixels/TP height
    96,  // x screen DPI
    96,  // y screen DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    2,  // max fingers
    5,  // max touch
    0,  // tripletap
    0,  // semi-mt
    1,  // is button pad
    0   // has_wheel
  };

  FingerState finger_states[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID
    // Consistent movement for 4 frames
    {0, 0, 0, 0, 20, 0, 10, 95, 1, 0},
    {0, 0, 0, 0, 20, 0, 59, 95, 2, 0},

    {0, 0, 0, 0, 20, 0, 10, 85, 1, 0},
    {0, 0, 0, 0, 20, 0, 59, 85, 2, 0},

    {0, 0, 0, 0, 20, 0, 10, 75, 1, 0},
    {0, 0, 0, 0, 20, 0, 59, 75, 2, 0},

    // Just too far apart to be scrolling
    {0, 0, 0, 0, 20, 0, 10, 65, 1, 0},
    {0, 0, 0, 0, 20, 0, 61, 65, 2, 0},
  };
  HardwareState hardware_states[] = {
    // time, buttons, finger count, touch count, finger states pointer
    { 1.00, 0, 2, 2, &finger_states[0], 0, 0, 0, 0 },
    { 2.00, 0, 2, 2, &finger_states[0], 0, 0, 0, 0 },
    { 2.01, 0, 2, 2, &finger_states[2], 0, 0, 0, 0 },
    { 2.02, 0, 2, 2, &finger_states[4], 0, 0, 0, 0 },
    { 2.03, 0, 2, 2, &finger_states[6], 0, 0, 0, 0 },
  };

  TestInterpreterWrapper wrapper(&ii, &hwprops);

  size_t idx = 0;

  // Consistent movement
  EXPECT_EQ(NULL, wrapper.SyncInterpret(&hardware_states[idx++], NULL));
  EXPECT_EQ(NULL, wrapper.SyncInterpret(&hardware_states[idx++], NULL));

  Gesture* gs = wrapper.SyncInterpret(&hardware_states[idx++], NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), gs);
  EXPECT_EQ(kGestureTypeScroll, gs->type);

  gs = wrapper.SyncInterpret(&hardware_states[idx++], NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), gs);
  EXPECT_EQ(kGestureTypeScroll, gs->type);

  gs = wrapper.SyncInterpret(&hardware_states[idx++], NULL);
  if (gs) {
    fprintf(stderr, "gs:%si=%zd\n", gs->String().c_str(), idx);
    EXPECT_NE(kGestureTypeScroll, gs->type);
  }
}


// This is based on a log from Dave Moore. He put one finger down, which put
// it into move mode, then put a second finger down a bit later, but it was
// stuck in move mode. This tests that it does switch to scroll mode.
TEST(ImmediateInterpreterTest, OneFingerThenTwoDelayedStartScrollTest) {
  ImmediateInterpreter ii(NULL, NULL);
  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    100,  // right edge
    100,  // bottom edge
    1,  // pixels/TP width
    1,  // pixels/TP height
    96,  // x screen DPI
    96,  // y screen DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    2,  // max fingers
    5,  // max touch
    0,  // tripletap
    0,  // semi-mt
    1,  // is button pad
    0   // has_wheel
  };

  FingerState finger_states[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID
    // Consistent movement for 4 frames
    {0, 0, 0, 0, 20, 0, 40, 85, 2, 0},

    {0, 0, 0, 0, 20, 0, 40, 85, 1, 0},
    {0, 0, 0, 0, 20, 0, 60, 83, 2, 0},

    {0, 0, 0, 0, 20, 0, 40, 77, 1, 0},
    {0, 0, 0, 0, 20, 0, 60, 75, 2, 0},

  };
  HardwareState hardware_states[] = {
    // time, buttons, finger count, touch count, finger states pointer
    { 1.00, 0, 1, 1, &finger_states[0], 0, 0, 0, 0 },
    { 1.20, 0, 2, 2, &finger_states[1], 0, 0, 0, 0 },
    { 2.00, 0, 2, 2, &finger_states[1], 0, 0, 0, 0 },
    { 2.01, 0, 2, 2, &finger_states[3], 0, 0, 0, 0 },
    { 2.03, 0, 0, 0, NULL, 0, 0, 0, 0 },
  };

  TestInterpreterWrapper wrapper(&ii, &hwprops);

  size_t idx = 0;

  // Consistent movement
  EXPECT_EQ(NULL, wrapper.SyncInterpret(&hardware_states[idx++], NULL));
  EXPECT_EQ(NULL, wrapper.SyncInterpret(&hardware_states[idx++], NULL));

  Gesture* gs = wrapper.SyncInterpret(&hardware_states[idx++], NULL);
  EXPECT_EQ(reinterpret_cast<Gesture*>(NULL), gs);

  gs = wrapper.SyncInterpret(&hardware_states[idx++], NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), gs);
  EXPECT_EQ(kGestureTypeScroll, gs->type);
}

namespace {

enum TestCaseStartOrContinueFlag {
  kS,  // start
  kC  // continue
};

enum OneFatFingerScrollTestExpectation {
  kAnything,
  kScroll
};

struct OneFatFingerScrollTestInputs {
  TestCaseStartOrContinueFlag start;
  stime_t now;
  float x0, y0, p0, x1, y1, p1;  // (x, y) coordinate and pressure
  OneFatFingerScrollTestExpectation expectation;
};

}  // namespace {}

// Tests two scroll operations with data from actual logs from Ryan Tabone.
TEST(ImmediateInterpreterTest, OneFatFingerScrollTest) {
  std::unique_ptr<ImmediateInterpreter> ii;
  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    106.666672,  // right edge
    68.000000,  // bottom edge
    1,  // pixels/TP width
    1,  // pixels/TP height
    25.4,  // screen DPI x
    25.4,  // screen DPI y
    -1,  // orientation minimum
    2,   // orientation maximum
    15,  // max fingers
    5,  // max touch
    0,  // t5r2
    0,  // semi-mt
    true,  // is button pad
    0   // has_wheel
  };
  TestInterpreterWrapper wrapper(ii.get(), &hwprops);
  // 4 runs that were failing, but now pass:
  OneFatFingerScrollTestInputs inputs[] = {
    { kS, 54.6787, 49.83, 33.20,  3.71, 73.25, 22.80, 32.82, kAnything },
    { kC, 54.6904, 49.83, 33.20, 61.93, 73.25, 22.80, 40.58, kAnything },
    { kC, 54.7022, 49.83, 33.20, 67.75, 73.25, 22.90, 40.58, kAnything },
    { kC, 54.7140, 49.83, 33.20, 67.75, 73.25, 22.90, 42.52, kAnything },
    { kC, 54.7256, 49.66, 33.20, 71.63, 73.25, 21.90, 38.64, kAnything },
    { kC, 54.7373, 49.00, 32.90, 75.51, 72.91, 20.80, 40.58, kAnything },
    { kC, 54.7492, 48.50, 31.70, 77.45, 72.75, 19.90, 40.58, kScroll },
    { kC, 54.7613, 47.91, 30.30, 77.45, 73.08, 17.90, 44.46, kScroll },
    { kC, 54.7734, 47.58, 26.80, 79.39, 73.08, 16.10, 46.40, kScroll },
    { kC, 54.7855, 47.33, 24.30, 85.21, 73.08, 13.40, 42.52, kScroll },
    { kC, 54.7976, 47.08, 21.30, 83.27, 73.25, 11.00, 46.40, kScroll },
    { kC, 54.8099, 47.08, 18.30, 87.15, 73.16,  9.00, 44.46, kScroll },
    { kC, 54.8222, 46.75, 15.90, 83.27, 73.16,  6.80, 42.52, kScroll },
    { kC, 54.8344, 46.66, 13.50, 85.21, 73.33,  4.80, 46.40, kScroll },
    { kC, 54.8469, 46.50, 11.80, 83.27, 73.33,  3.70, 44.46, kScroll },
    { kC, 54.8598, 46.41, 10.80, 85.21, 73.33,  3.00, 46.40, kScroll },
    { kC, 54.8726, 46.00,  9.50, 79.39, 73.33,  1.70, 40.58, kScroll },
    { kC, 54.8851, 46.00,  8.60, 81.33, 73.33,  1.50, 40.58, kScroll },
    { kC, 54.8975, 46.00,  7.90, 83.27, 73.33,  1.20, 38.64, kScroll },
    { kC, 54.9099, 46.00,  7.20, 85.21, 73.33,  1.20, 38.64, kScroll },
    { kC, 54.9224, 46.00,  7.00, 81.33, 73.33,  1.00, 34.76, kScroll },
    { kC, 54.9350, 46.00,  7.00, 81.33, 73.66,  0.90, 34.76, kScroll },
    { kC, 54.9473, 46.00,  6.80, 83.27, 73.66,  0.50, 34.76, kScroll },
    { kC, 54.9597, 46.00,  6.70, 77.45, 73.66,  0.40, 32.82, kScroll },
    { kC, 54.9721, 46.00,  6.60, 56.10, 73.50,  0.40, 28.94, kScroll },
    { kC, 54.9844, 46.41,  6.20, 32.82, 73.16,  0.40, 19.24, kScroll },
    { kC, 54.9967, 46.08,  6.20, 17.30, 72.41,  0.40,  7.60, kScroll },
    { kC, 55.0067, 47.16,  6.30,  3.71,  0.00,  0.00,  0.00, kAnything },

    { kS, 91.6606, 48.08, 31.20,  9.54,  0.00,  0.00,  0.00, kAnything },
    { kC, 91.6701, 48.08, 31.20, 23.12,  0.00,  0.00,  0.00, kAnything },
    { kC, 91.6821, 48.25, 31.20, 38.64, 69.50, 23.20,  7.60, kAnything },
    { kC, 91.6943, 48.25, 31.20, 50.28, 69.50, 23.20, 19.24, kAnything },
    { kC, 91.7062, 48.25, 31.20, 58.04, 69.41, 23.00, 23.12, kAnything },
    { kC, 91.7182, 48.25, 31.20, 63.87, 69.41, 23.00, 27.00, kAnything },
    { kC, 91.7303, 48.25, 31.20, 65.81, 69.16, 23.00, 30.88, kAnything },
    { kC, 91.7423, 48.25, 31.20, 65.81, 69.08, 23.00, 30.88, kAnything },
    { kC, 91.7541, 48.25, 31.20, 67.75, 69.83, 21.90, 25.06, kAnything },
    { kC, 91.7660, 48.25, 30.80, 67.75, 69.75, 21.90, 27.00, kAnything },
    { kC, 91.7778, 48.25, 30.00, 63.87, 69.75, 21.60, 30.88, kAnything },
    { kC, 91.7895, 48.25, 29.00, 63.87, 69.75, 21.30, 30.88, kAnything },
    { kC, 91.8016, 48.25, 27.60, 65.81, 69.50, 19.90, 34.76, kAnything },
    { kC, 91.8138, 48.16, 26.00, 67.75, 69.41, 18.70, 36.70, kScroll },
    { kC, 91.8259, 47.83, 24.30, 69.69, 69.16, 17.50, 40.58, kScroll },
    { kC, 91.8382, 47.66, 22.50, 69.69, 69.16, 15.50, 36.70, kScroll },
    { kC, 91.8503, 47.58, 19.20, 71.63, 69.16, 13.20, 34.76, kScroll },
    { kC, 91.8630, 47.41, 17.10, 71.63, 69.16, 10.80, 40.58, kScroll },
    { kC, 91.8751, 47.16, 14.70, 73.57, 69.16,  8.40, 34.76, kScroll },
    { kC, 91.8871, 47.16, 12.70, 73.57, 69.50,  7.10, 36.70, kScroll },
    { kC, 91.8994, 47.16, 11.30, 71.63, 69.75,  5.90, 36.70, kScroll },
    { kC, 91.9119, 47.16, 10.10, 67.75, 69.75,  4.40, 40.58, kScroll },
    { kC, 91.9243, 47.58,  8.70, 69.69, 69.75,  3.50, 42.52, kScroll },
    { kC, 91.9367, 48.00,  7.80, 63.87, 70.08,  2.70, 38.64, kScroll },
    { kC, 91.9491, 48.33,  6.90, 59.99, 70.58,  2.10, 34.76, kScroll },
    { kC, 91.9613, 48.66,  6.50, 56.10, 70.58,  1.50, 32.82, kScroll },
    { kC, 91.9732, 48.91,  6.00, 48.34, 70.66,  1.10, 28.94, kScroll },
    { kC, 91.9854, 49.00,  5.90, 38.64, 71.00,  1.10, 23.12, kScroll },
    { kC, 91.9975, 49.41,  5.60, 27.00, 71.33,  1.10, 15.36, kScroll },
    { kC, 92.0094, 49.41,  5.30, 13.42, 71.33,  0.90,  9.54, kScroll },
    { kC, 92.0215, 49.33,  4.20,  7.60, 71.33,  0.50,  3.71, kScroll },

    { kS, 93.3635, 43.58, 31.40, 36.70, 60.75, 19.00, 11.48, kAnything },
    { kC, 93.3757, 43.58, 31.40, 73.57, 60.58, 18.80, 27.00, kAnything },
    { kC, 93.3880, 43.58, 31.40, 75.51, 60.41, 17.90, 32.82, kAnything },
    { kC, 93.4004, 43.33, 31.20, 77.45, 60.33, 17.40, 38.64, kAnything },
    { kC, 93.4126, 43.00, 30.70, 79.39, 60.33, 16.50, 42.52, kAnything },
    { kC, 93.4245, 42.75, 28.90, 81.33, 60.33, 15.70, 46.40, kScroll },
    { kC, 93.4364, 42.41, 27.00, 79.39, 60.33, 14.30, 48.34, kScroll },
    { kC, 93.4485, 42.16, 25.80, 87.15, 60.33, 12.50, 50.28, kScroll },
    { kC, 93.4609, 42.08, 24.20, 89.09, 60.33, 11.10, 56.10, kScroll },
    { kC, 93.4733, 41.66, 21.70, 81.33, 60.33,  9.70, 52.22, kScroll },
    { kC, 93.4855, 41.66, 18.50, 85.21, 60.33,  7.80, 52.22, kScroll },
    { kC, 93.4978, 41.66, 16.29, 85.21, 60.66,  5.40, 54.16, kScroll },
    { kC, 93.5104, 41.66, 13.20, 79.39, 60.75,  3.80, 54.16, kScroll },
    { kC, 93.5227, 41.66, 11.80, 79.39, 62.33,  2.00, 42.52, kScroll },
    { kC, 93.5350, 41.91, 10.60, 71.63, 61.58,  1.80, 42.52, kScroll },
    { kC, 93.5476, 42.00,  9.10, 67.75, 61.83,  1.20, 38.64, kScroll },
    { kC, 93.5597, 42.41,  7.70, 58.04, 61.83,  0.80, 32.82, kScroll },
    { kC, 93.5718, 42.41,  7.20, 48.34, 61.83,  0.80, 27.00, kScroll },
    { kC, 93.5837, 42.33,  6.80, 34.76, 62.08,  0.50, 19.24, kScroll },
    { kC, 93.5957, 42.00,  6.10, 19.24, 62.08,  0.50, 15.36, kScroll },
    { kC, 93.6078, 41.91,  6.30,  7.60, 62.08,  0.50,  5.65, kAnything },

    { kS, 95.4803, 65.66, 34.90, 13.42,  0.00,  0.00,  0.00, kAnything },
    { kC, 95.4901, 66.00, 35.00, 36.70,  0.00,  0.00,  0.00, kAnything },
    { kC, 95.5024, 66.00, 35.10, 40.58, 44.66, 45.29, 59.99, kAnything },
    { kC, 95.5144, 66.00, 35.40, 38.64, 44.66, 45.29, 81.33, kAnything },
    { kC, 95.5267, 66.00, 35.40, 38.64, 44.50, 45.29, 87.15, kAnything },
    { kC, 95.5388, 66.00, 35.40, 40.58, 44.50, 45.29, 87.15, kAnything },
    { kC, 95.5507, 66.00, 33.60, 38.64, 44.50, 45.29, 91.03, kAnything },
    { kC, 95.5625, 65.75, 32.00, 34.76, 44.08, 43.60, 91.03, kScroll },
    { kC, 95.5747, 66.75, 30.00, 42.52, 43.83, 42.00, 89.09, kScroll },
    { kC, 95.5866, 66.75, 27.50, 38.64, 43.58, 38.90, 87.15, kScroll },
    { kC, 95.5986, 66.75, 25.00, 44.46, 43.58, 36.50, 92.97, kScroll },
    { kC, 95.6111, 66.75, 22.70, 42.52, 43.33, 33.70, 89.09, kScroll },
    { kC, 95.6230, 67.16, 20.40, 42.52, 43.33, 31.30, 94.91, kScroll },
    { kC, 95.6351, 67.33, 18.70, 44.46, 43.33, 28.90, 96.85, kScroll },
    { kC, 95.6476, 67.50, 17.30, 48.34, 43.33, 26.10, 92.97, kScroll },
    { kC, 95.6596, 67.83, 16.20, 46.40, 43.33, 25.00, 92.97, kScroll },
    { kC, 95.6717, 67.83, 15.60, 42.52, 43.33, 24.20, 94.91, kScroll },
    { kC, 95.6837, 68.00, 13.80, 46.40, 43.33, 23.90, 92.97, kScroll },
    { kC, 95.6959, 68.00, 13.80, 44.46, 43.33, 23.70, 92.97, kScroll },
    { kC, 95.7080, 68.00, 13.80, 44.46, 43.33, 23.50, 94.91, kScroll },
    { kC, 95.7199, 68.00, 13.60, 44.46, 43.33, 23.10, 96.85, kScroll },
    { kC, 95.7321, 68.00, 13.60, 44.46, 43.33, 23.00, 98.79, kScroll },
    { kC, 95.7443, 68.25, 13.60, 44.46, 43.25, 23.00, 98.79, kScroll },
  };
  for (size_t i = 0; i < arraysize(inputs); i++) {
    if (inputs[i].start == kS) {
      ii.reset(new ImmediateInterpreter(NULL, NULL));
      wrapper.Reset(ii.get());
    }

    FingerState fs[] = {
      { 0, 0, 0, 0, inputs[i].p0, 0.0, inputs[i].x0, inputs[i].y0, 1, 0 },
      { 0, 0, 0, 0, inputs[i].p1, 0.0, inputs[i].x1, inputs[i].y1, 2, 0 },
    };
    unsigned short finger_cnt = inputs[i].p1 == 0.0 ? 1 : 2;
    HardwareState hs = { inputs[i].now,0,finger_cnt,finger_cnt,fs,0,0,0,0 };

    stime_t timeout = -1.0;
    Gesture* gs = wrapper.SyncInterpret(&hs, &timeout);
    switch (inputs[i].expectation) {
      case kAnything:
        // Anything goes
        break;
      case kScroll:
        EXPECT_NE(static_cast<Gesture*>(NULL), gs) << "i=" << i;
        if (!gs)
          break;
        EXPECT_EQ(kGestureTypeScroll, gs->type);
        break;
    }
  }
};

struct NoLiftoffScrollTestInputs {
  bool reset;
  stime_t now;
  float x0, y0, p0, x1, y1, p1;  // (x, y) coordinate and pressure per finger
};

// Tests that if one scrolls backwards a bit before lifting fingers off, we
// don't scroll backwards. Based on an actual log
TEST(ImmediateInterpreterTest, NoLiftoffScrollTest) {
  std::unique_ptr<ImmediateInterpreter> ii;
  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    106.666672,  // right edge
    68.000000,  // bottom edge
    1,  // pixels/TP width
    1,  // pixels/TP height
    25.4,  // screen DPI x
    25.4,  // screen DPI y
    -1,  // orientation minimum
    2,   // orientation maximum
    15,  // max fingers
    5,  // max touch
    0,  // t5r2
    0,  // semi-mt
    true,  // is button pad
    0   // has_wheel
  };
  TestInterpreterWrapper wrapper(ii.get(), &hwprops);

  NoLiftoffScrollTestInputs inputs[] = {
    // These logs are examples of scrolling up that may have some accidental
    // reverse-scroll when fingers lift-off
    {  true, 4.9621, 59.5, 55.9, 17.30, 43.2, 62.5, 19.24 },
    { false, 4.9745, 59.5, 55.9, 30.88, 43.2, 62.5, 25.06 },
    { false, 4.9862, 59.3, 55.9, 34.76, 43.3, 61.7, 28.94 },
    { false, 4.9974, 59.3, 55.4, 36.70, 43.0, 60.7, 32.82 },
    { false, 5.0085, 59.0, 54.4, 40.58, 43.0, 58.7, 36.70 },
    { false, 5.0194, 59.0, 50.9, 44.46, 42.5, 55.7, 42.52 },
    { false, 5.0299, 59.0, 48.2, 46.40, 42.2, 52.7, 44.46 },
    { false, 5.0412, 58.7, 44.5, 46.40, 41.6, 49.7, 48.34 },
    { false, 5.0518, 57.3, 39.6, 48.34, 41.2, 45.7, 54.16 },
    { false, 5.0626, 57.1, 35.2, 48.34, 41.0, 42.0, 61.93 },
    { false, 5.0739, 56.7, 30.8, 56.10, 41.1, 36.6, 69.69 },
    { false, 5.0848, 56.3, 26.4, 58.04, 39.7, 32.3, 63.87 },
    { false, 5.0957, 56.3, 23.4, 61.93, 39.7, 27.8, 67.75 },
    { false, 5.1068, 56.3, 19.9, 67.75, 39.7, 24.1, 71.63 },
    { false, 5.1177, 56.7, 18.1, 71.63, 39.7, 20.4, 75.51 },
    { false, 5.1287, 57.1, 15.9, 71.63, 39.7, 18.7, 75.51 },
    { false, 5.1398, 57.5, 14.2, 77.45, 39.7, 17.3, 79.39 },
    { false, 5.1508, 57.6, 13.3, 75.51, 39.7, 16.1, 77.45 },
    { false, 5.1619, 57.7, 12.9, 79.39, 40.0, 15.5, 83.27 },
    { false, 5.1734, 58.1, 12.8, 79.39, 40.0, 15.4, 83.27 },
    { false, 5.1847, 58.1, 12.7, 79.39, 40.0, 15.3, 83.27 },
    { false, 5.1963, 58.1, 12.7, 78.42, 40.0, 15.3, 83.27 },
    { false, 5.2078, 58.1, 12.7, 77.45, 40.0, 15.3, 83.27 },
    { false, 5.2191, 58.1, 12.7, 79.39, 40.0, 15.3, 83.27 },
    { false, 5.2306, 58.1, 12.7, 78.42, 40.0, 15.3, 82.30 },
    { false, 5.2421, 58.1, 12.7, 77.45, 40.0, 15.3, 81.33 },
    { false, 5.2533, 58.1, 12.7, 77.45, 40.0, 15.3, 77.45 },
    { false, 5.2642, 58.1, 12.7, 63.87, 40.0, 15.4, 58.04 },
    { false, 5.2752, 57.9, 12.7, 34.76, 40.0, 15.8, 25.06 },

    {  true, 4.1501, 66.25, 19.10, 46.40, 83.50, 15.10, 46.40 },
    { false, 4.1610, 66.25, 19.00, 48.34, 83.58, 15.10, 46.40 },
    { false, 4.1721, 66.58, 18.50, 48.34, 83.58, 15.00, 44.46 },
    { false, 4.1830, 67.00, 18.50, 48.34, 83.66, 14.90, 44.46 },
    { false, 4.1943, 67.08, 18.40, 50.28, 83.66, 14.80, 46.40 },
    { false, 4.2053, 67.08, 18.40, 50.28, 83.66, 14.80, 46.40 },
    { false, 4.2163, 67.08, 18.40, 50.28, 83.66, 14.80, 46.40 },
    { false, 4.2274, 67.08, 18.40, 48.34, 83.66, 14.80, 46.40 },
    { false, 4.2385, 67.08, 18.30, 50.28, 83.83, 14.60, 46.40 },
    { false, 4.2494, 67.08, 18.10, 48.34, 83.91, 14.30, 46.40 },
    { false, 4.2602, 67.08, 17.60, 46.40, 84.08, 14.10, 44.46 },
    { false, 4.2712, 67.08, 17.40, 48.34, 84.25, 13.70, 46.40 },
    { false, 4.2822, 67.25, 17.20, 48.34, 84.50, 13.40, 48.34 },
    { false, 4.2932, 67.33, 16.90, 46.40, 84.75, 13.20, 46.40 },
    { false, 4.3044, 67.33, 16.60, 46.40, 84.91, 13.00, 48.34 },
    { false, 4.3153, 67.41, 16.50, 46.40, 84.91, 12.90, 46.40 },
    { false, 4.3264, 67.50, 16.29, 46.40, 84.91, 12.90, 46.40 },
    { false, 4.3372, 67.58, 16.29, 46.40, 85.08, 12.90, 48.34 },
    { false, 4.3481, 67.58, 16.10, 44.46, 85.08, 12.90, 48.34 },
    { false, 4.3591, 67.58, 16.00, 44.46, 85.08, 12.90, 48.34 },
    { false, 4.3699, 67.58, 15.95, 44.46, 85.08, 12.85, 48.34 },
    { false, 4.3808, 67.58, 15.90, 44.46, 85.08, 12.80, 48.34 },
    { false, 4.3922, 67.58, 15.90, 44.46, 85.25, 12.50, 48.34 },
    { false, 4.4035, 67.75, 15.80, 46.40, 85.25, 12.40, 46.40 },
    { false, 4.4146, 67.75, 15.30, 46.40, 85.33, 12.20, 48.34 },
    { false, 4.4260, 67.91, 15.20, 48.34, 85.75, 12.20, 50.28 },
    { false, 4.4373, 67.91, 15.20, 46.40, 85.75, 12.10, 48.34 },
    { false, 4.4485, 67.91, 15.10, 46.40, 85.75, 12.10, 48.34 },
    { false, 4.4712, 67.91, 15.05, 46.40, 85.75, 12.05, 48.34 },
    { false, 4.4940, 67.91, 15.00, 46.40, 85.75, 12.00, 48.34 },
    { false, 4.5052, 67.91, 14.80, 48.34, 85.75, 11.80, 48.34 },
    { false, 4.5163, 68.00, 14.60, 48.34, 85.83, 11.70, 48.34 },
    { false, 4.5276, 68.08, 14.50, 48.34, 85.91, 11.60, 50.28 },
    { false, 4.5390, 68.08, 14.30, 46.40, 85.91, 11.50, 48.34 },
    { false, 4.5499, 68.08, 14.30, 48.34, 85.91, 11.50, 48.34 },
    { false, 4.5613, 68.08, 14.30, 47.37, 85.91, 11.45, 48.34 },
    { false, 4.5726, 68.08, 14.30, 46.40, 85.91, 11.40, 48.34 },
    { false, 4.5837, 68.08, 14.20, 46.40, 85.91, 11.40, 48.34 },
    { false, 4.5949, 68.08, 14.10, 46.40, 85.91, 11.40, 48.34 },
    { false, 4.6061, 68.16, 14.10, 46.40, 85.91, 11.40, 48.34 },
    { false, 4.6172, 68.16, 14.00, 48.34, 86.00, 11.30, 48.34 },
    { false, 4.6285, 68.25, 13.90, 48.34, 86.00, 11.20, 48.34 },
    { false, 4.6399, 68.25, 13.90, 48.34, 86.00, 11.20, 48.34 },
    { false, 4.6514, 68.33, 13.80, 48.34, 86.00, 11.10, 48.34 },
    { false, 4.6741, 68.33, 13.80, 47.37, 86.00, 11.05, 47.37 },
    { false, 4.6968, 68.33, 13.80, 46.40, 86.00, 11.00, 46.40 },
    { false, 4.7079, 68.33, 13.80, 42.52, 86.00, 11.00, 44.46 },
    { false, 4.7191, 68.33, 13.80, 38.64, 86.00, 11.00, 42.52 },
    { false, 4.7304, 68.33, 13.80, 34.76, 86.00, 11.00, 42.52 },
    { false, 4.7417, 68.41, 13.80, 27.00, 86.41, 11.00, 36.70 },
    { false, 4.7528, 68.83, 13.60, 21.18, 86.25, 10.90, 32.82 },
    { false, 4.7638, 68.83, 13.60, 13.42, 86.25, 10.80, 25.06 },
    { false, 4.7749, 68.83, 13.60,  5.65, 86.25, 10.50, 15.36 },
    { false, 4.7862, 68.75, 14.00,  1.77, 85.91, 10.50,  7.60 },
  };
  for (size_t i = 0; i < arraysize(inputs); i++) {
    if (inputs[i].reset) {
      ii.reset(new ImmediateInterpreter(NULL, NULL));
      wrapper.Reset(ii.get());
    }
    FingerState fs[] = {
      { 0, 0, 0, 0, inputs[i].p0, 0.0, inputs[i].x0, inputs[i].y0, 1, 0 },
      { 0, 0, 0, 0, inputs[i].p1, 0.0, inputs[i].x1, inputs[i].y1, 2, 0 },
    };
    HardwareState hs = { inputs[i].now, 0, 2, 2, fs, 0, 0, 0, 0 };

    stime_t timeout = -1.0;
    Gesture* gs = wrapper.SyncInterpret(&hs, &timeout);
    if (gs) {
      EXPECT_EQ(kGestureTypeScroll, gs->type);
      EXPECT_LE(gs->details.scroll.dy, 0.0);
    }
  }
}

struct HardwareStateAnScrollExpectations {
  HardwareState hs;
  float dx;
  float dy;
};

TEST(ImmediateInterpreterTest, DiagonalSnapTest) {
  std::unique_ptr<ImmediateInterpreter> ii;
  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    100,  // right edge
    100,  // bottom edge
    1,  // pixels/TP width
    1,  // pixels/TP height
    96,  // x screen DPI
    96,  // y screen DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    2,  // max fingers
    5,  // max touch
    0,  // t5r2
    0,  // semi-mt
    1,  // is button pad
    0   // has_wheel
  };
  TestInterpreterWrapper wrapper(ii.get(), &hwprops);

  const float kBig = 5;  // mm
  const float kSml = 1;  // mm

  const float kX0 = 40;
  const float kX1 = 60;
  const float kY = 50;  // heh

  short fid = 1;

  FingerState finger_states[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID

    // Perfect diagonal movement - should scroll diagonally
    {0, 0, 0, 0, 50, 0, kX0, kY, fid++, 0},
    {0, 0, 0, 0, 50, 0, kX1, kY, fid--, 0},

    {0, 0, 0, 0, 50, 0, kX0 + kBig, kY + kBig, fid++, 0},
    {0, 0, 0, 0, 50, 0, kX1 + kBig, kY + kBig, fid++, 0},

    // Almost vertical movement - should snap to vertical
    {0, 0, 0, 0, 50, 0, kX0, kY, fid++, 0},
    {0, 0, 0, 0, 50, 0, kX1, kY, fid--, 0},

    {0, 0, 0, 0, 50, 0, kX0 + kSml, kY + kBig, fid++, 0},
    {0, 0, 0, 0, 50, 0, kX1 + kSml, kY + kBig, fid++, 0},

    // Almost horizontal movement - should snap to horizontal
    {0, 0, 0, 0, 50, 0, kX0, kY, fid++, 0},
    {0, 0, 0, 0, 50, 0, kX1, kY, fid--, 0},

    {0, 0, 0, 0, 50, 0, kX0 + kBig, kY + kSml, fid++, 0},
    {0, 0, 0, 0, 50, 0, kX1 + kBig, kY + kSml, fid++, 0},

    // Vertical movement with Warp - shouldn't scroll
    {0, 0, 0, 0, 50, 0, kX0, kY, fid++, 0},
    {0, 0, 0, 0, 50, 0, kX1, kY, fid--, 0},

    {0, 0, 0, 0, 50, 0, kX0, kY + kBig, fid++, GESTURES_FINGER_WARP_Y},
    {0, 0, 0, 0, 50, 0, kX1, kY + kBig, fid++, GESTURES_FINGER_WARP_Y},
  };
  ssize_t idx = 0;
  HardwareStateAnScrollExpectations hardware_states[] = {
    // time, buttons, finger count, touch count, finger states pointer
    { { 0.000, 0, 2, 2, &finger_states[idx * 4 ], 0, 0, 0, 0 }, 0, 0 },
    { { 1.000, 0, 2, 2, &finger_states[idx * 4 ], 0, 0, 0, 0 }, 0, 0 },
    { { 1.010,0,2,2,&finger_states[idx++ * 4 + 2],0,0,0,0 },kBig,kBig },

    { { 0.000, 0, 2, 2, &finger_states[idx * 4 ], 0, 0, 0, 0 }, 0, 0 },
    { { 1.000, 0, 2, 2, &finger_states[idx * 4 ], 0, 0, 0, 0 }, 0, 0 },
    { { 1.010, 0, 2, 2, &finger_states[idx++ * 4 + 2], 0, 0, 0, 0 }, 0, kBig },

    { { 0.000, 0, 2, 2, &finger_states[idx * 4 ], 0, 0, 0, 0 }, 0, 0 },
    { { 1.000, 0, 2, 2, &finger_states[idx * 4 ], 0, 0, 0, 0 }, 0, 0 },
    { { 1.010, 0, 2, 2, &finger_states[idx++ * 4 + 2], 0, 0, 0, 0 }, kBig, 0 },

    { { 0.000, 0, 2, 2, &finger_states[idx * 4 ], 0, 0, 0, 0 }, 0, 0 },
    { { 1.000, 0, 2, 2, &finger_states[idx * 4 ], 0, 0, 0, 0 }, 0, 0 },
    { { 1.010, 0, 2, 2, &finger_states[idx++ * 4 + 2], 0, 0, 0, 0 }, 0, 0 },
  };

  for (size_t i = 0; i < arraysize(hardware_states); i++) {
    HardwareStateAnScrollExpectations& hse = hardware_states[i];
    if (hse.hs.timestamp == 0.0) {
      ii.reset(new ImmediateInterpreter(NULL, NULL));
      wrapper.Reset(ii.get());
    }
    Gesture* gs = wrapper.SyncInterpret(&hse.hs, NULL);
    if (hse.dx == 0.0 && hse.dy == 0.0) {
      EXPECT_EQ(reinterpret_cast<Gesture*>(NULL), gs);
      continue;
    }
    ASSERT_NE(reinterpret_cast<Gesture*>(NULL), gs);
    EXPECT_EQ(kGestureTypeScroll, gs->type);
    EXPECT_FLOAT_EQ(hse.dx, gs->details.scroll.dx);
    EXPECT_FLOAT_EQ(hse.dy, gs->details.scroll.dy);
  }
}

TEST(ImmediateInterpreterTest, RestingFingerTest) {
  std::unique_ptr<ImmediateInterpreter> ii;
  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    100,  // right edge
    100,  // bottom edge
    1,  // pixels/TP width
    1,  // pixels/TP height
    96,  // x screen DPI
    96,  // y screen DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    2,  // max fingers
    5,  // max touch
    0,  // t5r2
    0,  // semi-mt
    1,  // is button pad
    0   // has_wheel
  };
  TestInterpreterWrapper wrapper(ii.get(), &hwprops);

  const float kX = 7;
  float dx = 7;
  const float kRestY = hwprops.bottom - 7;
  const float kMoveY = kRestY - 10;

  const float kTO = 1.0;  // time to wait for change timeout

  FingerState finger_states[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID

    // Resting finger in lower left
    {0, 0, 0, 0, 50, 0, kX, kRestY, 1, 0},
    // Moving finger
    {0, 0, 0, 0, 50, 0, kX, kMoveY, 2, 0},
  };

  // Left to right movement, then right to left
  for (size_t direction = 0; direction < 2; direction++) {
    if (direction == 1)
      dx *= -1.0;
    ii.reset(new ImmediateInterpreter(NULL, NULL));
    wrapper.Reset(ii.get());
    for (size_t i = 0; i < 4; i++) {
      HardwareState hs = { kTO + 0.01 * i, 0, 2, 2, finger_states, 0, 0, 0, 0 };
      if (i == 0) {
        hs.timestamp -= kTO;
        Gesture* gs = wrapper.SyncInterpret(&hs, NULL);
        EXPECT_EQ(static_cast<Gesture*>(NULL), gs);
        hs.timestamp += kTO;
        gs = wrapper.SyncInterpret(&hs, NULL);
        if (gs && gs->type == kGestureTypeMove) {
          EXPECT_FLOAT_EQ(0.0, gs->details.move.dx);
          EXPECT_FLOAT_EQ(0.0, gs->details.move.dy);
        }
      } else {
        Gesture* gs = wrapper.SyncInterpret(&hs, NULL);
        ASSERT_NE(static_cast<Gesture*>(NULL), gs);
        EXPECT_EQ(kGestureTypeMove, gs->type);
        EXPECT_FLOAT_EQ(dx, gs->details.move.dx);
        EXPECT_FLOAT_EQ(0.0, gs->details.move.dy);
      }
      finger_states[1].position_x += dx;
    }
  }
}

TEST(ImmediateInterpreterTest, ThumbRetainTest) {
  ImmediateInterpreter ii(NULL, NULL);
  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    100,  // right edge
    100,  // bottom edge
    1,  // pixels/TP width
    1,  // pixels/TP height
    1,  // x screen DPI
    1,  // y screen DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    2,  // max fingers
    5,  // max touch
    0,  // tripletap
    0,  // semi-mt
    1,  // is button pad
    0   // has_wheel
  };

  FingerState finger_states[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID
    // id 1 = finger, 2 = thumb
    {0, 0, 0, 0, 24, 0, 30, 30, 1, 0},
    {0, 0, 0, 0, 58, 0, 30, 50, 2, 0},

    // thumb, post-move
    {0, 0, 0, 0, 58, 0, 50, 50, 2, 0},
  };
  HardwareState hardware_states[] = {
    // time, buttons, finger count, touch count, finger states pointer
    { 0.000, 0, 2, 2, &finger_states[0], 0, 0, 0, 0 },
    { 0.100, 0, 2, 2, &finger_states[0], 0, 0, 0, 0 },
    { 0.110, 0, 1, 1, &finger_states[1], 0, 0, 0, 0 },  // finger goes away
    { 0.210, 0, 1, 1, &finger_states[1], 0, 0, 0, 0 },
    { 0.220, 0, 1, 1, &finger_states[2], 0, 0, 0, 0 },  // thumb moves
  };

  TestInterpreterWrapper wrapper(&ii, &hwprops);
  ii.tap_enable_.val_ = 0;

  for (size_t i = 0; i < arraysize(hardware_states); i++) {
    Gesture* gs = wrapper.SyncInterpret(&hardware_states[i], NULL);
    if (!gs)
      continue;
    EXPECT_EQ(kGestureTypeMove, gs->type) << "i=" << i;
    EXPECT_FLOAT_EQ(0.0, gs->details.move.dx) << "i=" << i;
    EXPECT_FLOAT_EQ(0.0, gs->details.move.dy) << "i=" << i;
  }
}

TEST(ImmediateInterpreterTest, ThumbRetainReevaluateTest) {
  ImmediateInterpreter ii(NULL, NULL);
  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    100,  // right edge
    100,  // bottom edge
    1,  // pixels/TP width
    1,  // pixels/TP height
    1,  // x screen DPI
    1,  // y screen DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    2,  // max fingers
    5,  // max touch
    0,  // tripletap
    0,  // semi-mt
    1,  // is button pad
    0   // has_wheel
  };

  FingerState finger_states[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID
    // one thumb, one finger (it seems)
    {0, 0, 0, 0, 24, 0, 3.0, 3, 3, 0},
    {0, 0, 0, 0, 58, 0, 13.5, 3, 4, 0},
    // two big fingers, it turns out!
    {0, 0, 0, 0, 27, 0, 3.0, 6, 3, 0},
    {0, 0, 0, 0, 58, 0, 13.5, 6, 4, 0},
    // they  move
    {0, 0, 0, 0, 27, 0, 3.0, 7, 3, 0},
    {0, 0, 0, 0, 58, 0, 13.5, 7, 4, 0},
  };
  HardwareState hardware_states[] = {
    // time, buttons, finger count, touch count, finger states pointer
    { 1.000, 0, 2, 2, &finger_states[0], 0, 0, 0, 0 },  // next 2 fingers arrive
    { 1.010, 0, 2, 2, &finger_states[2], 0, 0, 0, 0 },  // pressures fix
    { 1.100, 0, 2, 2, &finger_states[4], 0, 0, 0, 0 },  // they move
  };

  TestInterpreterWrapper wrapper(&ii, &hwprops);
  ii.tap_enable_.val_ = 0;

  for (size_t i = 0; i < arraysize(hardware_states); i++) {
    Gesture* gs = wrapper.SyncInterpret(&hardware_states[i], NULL);
    EXPECT_TRUE(!gs || gs->type == kGestureTypeScroll);
  }
}

TEST(ImmediateInterpreterTest, SetHardwarePropertiesTwiceTest) {
  ImmediateInterpreter ii(NULL, NULL);
  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    1000,  // right edge
    1000,  // bottom edge
    500,  // pixels/TP width
    500,  // pixels/TP height
    96,  // screen DPI x
    96,  // screen DPI y
    -1,  // orientation minimum
    2,   // orientation maximum
    2,  // max fingers
    5,  // max touch
    0,  // tripletap
    0,  // semi-mt
    1,  // is button pad
    0   // has_wheel
  };
  hwprops.max_finger_cnt = 3;
  TestInterpreterWrapper wrapper(&ii, &hwprops);

  FingerState finger_states[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID
    {0, 0, 0, 0, 1, 0, 0, 0, 1, 0},
    {0, 0, 0, 0, 1, 0, 0, 0, 2, 0},
    {0, 0, 0, 0, 1, 0, 0, 0, 3, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 4, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 5, 0}
  };
  HardwareState hardware_state = {
    // time, buttons, finger count, touch count, finger states pointer
    200000, 0, 5, 5, &finger_states[0], 0, 0, 0, 0
  };
  // This used to cause a crash:
  Gesture* gs = wrapper.SyncInterpret(&hardware_state, NULL);
  EXPECT_EQ(reinterpret_cast<Gesture*>(NULL), gs);
}

TEST(ImmediateInterpreterTest, AmbiguousPalmCoScrollTest) {
  ImmediateInterpreter ii(NULL, NULL);
  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    100,  // right edge
    100,  // bottom edge
    1,  // x pixels/TP width
    1,  // y pixels/TP height
    1,  // x screen DPI
    1,  // y screen DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    5,  // max fingers
    5,  // max touch
    0,  // t5r2
    0,  // semi-mt
    1,  // is button pad
    0   // has_wheel
  };
  TestInterpreterWrapper wrapper(&ii, &hwprops);

  const int kPr = 20;

  const unsigned kPalmFlags = GESTURES_FINGER_POSSIBLE_PALM;

  FingerState finger_states[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID
    // stationary palm - movement
    {0, 0, 0, 0, kPr, 0,  0, 40, 1, kPalmFlags},
    {0, 0, 0, 0, kPr, 0, 30, 35, 2, 0},

    {0, 0, 0, 0, kPr, 0,  0, 40, 1, kPalmFlags},
    {0, 0, 0, 0, kPr, 0, 30, 40, 2, 0},

    {0, 0, 0, 0, kPr, 0,  0, 40, 1, kPalmFlags},
    {0, 0, 0, 0, kPr, 0, 30, 45, 2, 0},

    // Same, but moving palm - scroll
    {0, 0, 0, 0, kPr, 0,  0, 35, 3, kPalmFlags},
    {0, 0, 0, 0, kPr, 0, 30, 35, 4, 0},

    {0, 0, 0, 0, kPr, 0,  0, 40, 3, kPalmFlags},
    {0, 0, 0, 0, kPr, 0, 30, 40, 4, 0},

    {0, 0, 0, 0, kPr, 0,  0, 45, 3, kPalmFlags},
    {0, 0, 0, 0, kPr, 0, 30, 45, 4, 0},
  };
  HardwareState hardware_state[] = {
    // time, buttons, finger count, touch count, finger states pointer
    { 0.0, 0, 2, 2, &finger_states[0], 0, 0, 0, 0 },
    { 0.1, 0, 2, 2, &finger_states[2], 0, 0, 0, 0 },
    { 0.2, 0, 2, 2, &finger_states[4], 0, 0, 0, 0 },
    { 3.0, 0, 2, 2, &finger_states[6], 0, 0, 0, 0 },
    { 3.1, 0, 2, 2, &finger_states[8], 0, 0, 0, 0 },
    { 3.2, 0, 2, 2, &finger_states[10], 0, 0, 0, 0 },
  };
  GestureType expected_gs[] = {
    kGestureTypeNull,
    kGestureTypeNull,
    kGestureTypeMove,
    kGestureTypeNull,
    kGestureTypeScroll,
    kGestureTypeScroll
  };
  if (ii.pinch_enable_.val_)
    // Movement delay is longer when pinch is enabled
    expected_gs[2] = kGestureTypeNull;

  ASSERT_EQ(arraysize(expected_gs), arraysize(hardware_state));

  for (size_t i = 0; i < arraysize(hardware_state); ++i) {
    Gesture* gs = wrapper.SyncInterpret(&hardware_state[i], NULL);
    if (expected_gs[i] == kGestureTypeNull) {
      EXPECT_EQ(static_cast<Gesture*>(NULL), gs) << "gs:" << gs->String();
    } else {
      ASSERT_NE(static_cast<Gesture*>(NULL), gs);
      EXPECT_EQ(expected_gs[i], gs->type) << "i=" << i
                                          << " gs: " << gs->String();
    }
  }
}

TEST(ImmediateInterpreterTest, PressureChangeMoveTest) {
  ImmediateInterpreter ii(NULL, NULL);
  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    1000,  // right edge
    1000,  // bottom edge
    500,  // x pixels/TP width
    500,  // y pixels/TP height
    96,  // x screen DPI
    96,  // y screen DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    2,  // max fingers
    5,  // max touch
    0,  // t5r2
    0,  // semi-mt
    1,  // is button pad
    0   // has_wheel
  };
  TestInterpreterWrapper wrapper(&ii, &hwprops);

  const int kBig = 81;  // large pressure
  const int kSml = 50;  // small pressure

  FingerState finger_states[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID
    {0, 0, 0, 0, kSml, 0, 600, 300, 1, 0},
    {0, 0, 0, 0, kSml, 0, 600, 400, 1, 0},
    {0, 0, 0, 0, kBig, 0, 600, 500, 1, 0},
    {0, 0, 0, 0, kBig, 0, 600, 600, 1, 0},
  };
  HardwareState hardware_state[] = {
    // time, buttons, finger count, touch count, finger states pointer
    { 2000.00, 0, 1, 1, &finger_states[0], 0, 0, 0, 0 },
    { 2000.01, 0, 1, 1, &finger_states[1], 0, 0, 0, 0 },
    { 2000.02, 0, 1, 1, &finger_states[2], 0, 0, 0, 0 },
    { 2000.03, 0, 1, 1, &finger_states[3], 0, 0, 0, 0 },
  };

  for (size_t i = 0; i < arraysize(hardware_state); ++i) {
    Gesture* result = wrapper.SyncInterpret(&hardware_state[i], NULL);
    switch (i) {
      case 0:
      case 2:
        EXPECT_FALSE(result);
        break;
      case 1:  // fallthrough
      case 3:
        ASSERT_TRUE(result);
        EXPECT_EQ(kGestureTypeMove, result->type);
        break;
    }
  }
}

TEST(ImmediateInterpreterTest, GetGesturingFingersTest) {
  ImmediateInterpreter ii(NULL, NULL);
  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    1000,  // right edge
    1000,  // bottom edge
    500,  // pixels/TP width
    500,  // pixels/TP height
    96,  // screen DPI x
    96,  // screen DPI y
    -1,  // orientation minimum
    2,   // orientation maximum
    2,  // max fingers
    5,  // max touch
    0,  // t5r2
    0,  // semi-mt
    1,  // is button pad
    0   // has_wheel
  };
  TestInterpreterWrapper wrapper(&ii, &hwprops);

  FingerState finger_states[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID
    {0, 0, 0, 0, 1, 0, 61, 70, 91, 0},
    {0, 0, 0, 0, 1, 0, 62, 65, 92, 0},
    {0, 0, 0, 0, 1, 0, 62, 69, 93, 0},
    {0, 0, 0, 0, 1, 0, 62, 61, 94, 0}
  };
  HardwareState hardware_state[] = {
    // time, buttons, finger count, finger states pointer
    { 200000, 0, 0, 0, NULL, 0, 0, 0, 0 },
    { 200001, 0, 1, 1, &finger_states[0], 0, 0, 0, 0 },
    { 200002, 0, 2, 2, &finger_states[0], 0, 0, 0, 0 },
    { 200002, 0, 3, 3, &finger_states[0], 0, 0, 0, 0 },
    { 200002, 0, 4, 4, &finger_states[0], 0, 0, 0, 0 }
  };
  // few pointing fingers
  ii.ResetSameFingersState(hardware_state[0]);
  ii.UpdatePointingFingers(hardware_state[0]);
  EXPECT_TRUE(ii.GetGesturingFingers(hardware_state[0]).empty());

  ii.ResetSameFingersState(hardware_state[0]);
  ii.UpdatePointingFingers(hardware_state[1]);
  set<short, kMaxGesturingFingers> ids =
      ii.GetGesturingFingers(hardware_state[1]);
  EXPECT_EQ(1, ids.size());
  EXPECT_TRUE(ids.end() != ids.find(91));

  ii.ResetSameFingersState(hardware_state[0]);
  ii.UpdatePointingFingers(hardware_state[2]);
  ids = ii.GetGesturingFingers(hardware_state[2]);
  EXPECT_EQ(2, ids.size());
  EXPECT_TRUE(ids.end() != ids.find(91));
  EXPECT_TRUE(ids.end() != ids.find(92));

  ii.ResetSameFingersState(hardware_state[0]);
  ii.UpdatePointingFingers(hardware_state[3]);
  ids = ii.GetGesturingFingers(hardware_state[3]);
  EXPECT_EQ(3, ids.size());
  EXPECT_TRUE(ids.end() != ids.find(91));
  EXPECT_TRUE(ids.end() != ids.find(92));
  EXPECT_TRUE(ids.end() != ids.find(93));

  ii.ResetSameFingersState(hardware_state[0]);
  ii.UpdatePointingFingers(hardware_state[4]);
  ids = ii.GetGesturingFingers(hardware_state[4]);
  EXPECT_EQ(4, ids.size());
  EXPECT_TRUE(ids.end() != ids.find(91));
  EXPECT_TRUE(ids.end() != ids.find(92));
  EXPECT_TRUE(ids.end() != ids.find(93));
  EXPECT_TRUE(ids.end() != ids.find(94));

  // T5R2 test
  hwprops.supports_t5r2 = 1;
  wrapper.Reset(&ii, &hwprops);
  ii.ResetSameFingersState(hardware_state[0]);
  ii.UpdatePointingFingers(hardware_state[2]);
  ids = ii.GetGesturingFingers(hardware_state[2]);
  EXPECT_EQ(2, ids.size());
  EXPECT_TRUE(ids.end() != ids.find(91));
  EXPECT_TRUE(ids.end() != ids.find(92));
}

namespace {
set<short, kMaxGesturingFingers> MkSet() {
  return set<short, kMaxGesturingFingers>();
}
set<short, kMaxGesturingFingers> MkSet(short the_id) {
  set<short, kMaxGesturingFingers> ret;
  ret.insert(the_id);
  return ret;
}
set<short, kMaxGesturingFingers> MkSet(short id1, short id2) {
  set<short, kMaxGesturingFingers> ret;
  ret.insert(id1);
  ret.insert(id2);
  return ret;
}
set<short, kMaxGesturingFingers> MkSet(short id1, short id2, short id3) {
  set<short, kMaxGesturingFingers> ret;
  ret.insert(id1);
  ret.insert(id2);
  ret.insert(id3);
  return ret;
}
}  // namespace{}

TEST(ImmediateInterpreterTest, TapRecordTest) {
  ImmediateInterpreter ii(NULL, NULL);
  TapRecord tr(&ii);
  EXPECT_FALSE(tr.TapComplete());
  // two finger IDs:
  const short kF1 = 91;
  const short kF2 = 92;
  const float kTapMoveDist = 1.0;  // mm
  ii.tap_min_pressure_.val_ = 25;

  FingerState fs[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID
    {0, 0, 0, 0, 50, 0, 4, 4, kF1, 0},
    {0, 0, 0, 0, 75, 0, 4, 9, kF2, 0},
    {0, 0, 0, 0, 50, 0, 7, 4, kF1, 0}
  };
  HardwareState nullstate = { 0.0, 0, 0, 0, NULL, 0, 0, 0, 0 };
  HardwareState hw[] = {
    // time, buttons, finger count, finger states pointer
    { 0.0, 0, 1, 1, &fs[0], 0, 0, 0, 0 },
    { 0.1, 0, 2, 2, &fs[0], 0, 0, 0, 0 },
    { 0.2, 0, 1, 1, &fs[1], 0, 0, 0, 0 },
    { 0.3, 0, 2, 2, &fs[0], 0, 0, 0, 0 },
    { 0.4, 0, 1, 1, &fs[1], 0, 0, 0, 0 },
    { 0.5, 0, 1, 1, &fs[2], 0, 0, 0, 0 }
  };

  tr.Update(hw[0], nullstate, MkSet(kF1), MkSet(), MkSet());
  EXPECT_FALSE(tr.Moving(hw[0], kTapMoveDist));
  EXPECT_FALSE(tr.TapComplete());
  tr.Update(hw[1], hw[0], MkSet(), MkSet(), MkSet());
  EXPECT_FALSE(tr.Moving(hw[1], kTapMoveDist));
  EXPECT_FALSE(tr.TapComplete());
  tr.Update(hw[2], hw[1], MkSet(), MkSet(kF1), MkSet());
  EXPECT_FALSE(tr.Moving(hw[2], kTapMoveDist));
  EXPECT_TRUE(tr.TapComplete());
  EXPECT_EQ(GESTURES_BUTTON_LEFT, tr.TapType());

  tr.Clear();
  EXPECT_FALSE(tr.TapComplete());
  tr.Update(hw[2], hw[1], MkSet(kF2), MkSet(), MkSet());
  EXPECT_FALSE(tr.Moving(hw[2], kTapMoveDist));
  EXPECT_FALSE(tr.TapComplete());
  tr.Update(hw[3], hw[2], MkSet(kF1), MkSet(), MkSet(kF2));
  EXPECT_FALSE(tr.Moving(hw[3], kTapMoveDist));
  EXPECT_FALSE(tr.TapComplete());
  tr.Update(hw[4], hw[3], MkSet(), MkSet(kF1), MkSet());
  EXPECT_FALSE(tr.Moving(hw[4], kTapMoveDist));
  EXPECT_TRUE(tr.TapComplete());

  tr.Clear();
  EXPECT_FALSE(tr.TapComplete());
  tr.Update(hw[0], nullstate, MkSet(kF1), MkSet(), MkSet());
  tr.Update(hw[5], hw[4], MkSet(), MkSet(), MkSet());
  EXPECT_TRUE(tr.Moving(hw[5], kTapMoveDist));
  EXPECT_FALSE(tr.TapComplete());

  // This should log an error
  tr.Clear();
  tr.Update(hw[2], hw[1], MkSet(), MkSet(kF1), MkSet());
}

namespace {

const long HWStateFlagStart          = 0x01000000;  // Start
const long HWStateFlagContinue       = 0x02000000;  // Continue
// Start; also start of slow double tap test:
const long HWStateFlagStartDoubleTap = 0x04000000;
// Start; also start of T5R2 tests:
const long HWStateFlagStartT5R2      = 0x08000000;
// Start; also tap dragging is disabled this time
const long HWStateFlagStartNoDrag    = 0x10000000;
const long HWStateFlagMask           = 0xff000000;

#define S (__LINE__ + HWStateFlagStart)
#define C (__LINE__ + HWStateFlagContinue)
#define D (__LINE__ + HWStateFlagStartDoubleTap)
#define T (__LINE__ + HWStateFlagStartT5R2)
#define N (__LINE__ + HWStateFlagStartNoDrag)

struct HWStateGs {
  long line_number_and_flags;
  HardwareState hws;
  stime_t callback_now;
  set<short, kMaxGesturingFingers> gs;
  unsigned expected_down;
  unsigned expected_up;
  ImmediateInterpreter::TapToClickState expected_state;
  bool timeout;

  bool IsStart() {
    return line_number_and_flags & (HWStateFlagStart |
                                    HWStateFlagStartDoubleTap |
                                    HWStateFlagStartT5R2 |
                                    HWStateFlagStartNoDrag);
  }
  long LineNumber() {
    return line_number_and_flags & ~HWStateFlagMask;
  }
};

size_t NonT5R2States(const HWStateGs* states, size_t length) {
  for (size_t i = 0; i < length; i++)
    if (states[i].line_number_and_flags & HWStateFlagStartT5R2)
      return i;
  return length;
}

}  // namespace {}

// Shorter names so that HWStateGs defs take only 1 line each
typedef ImmediateInterpreter::TapToClickState TapState;
const TapState kIdl = ImmediateInterpreter::kTtcIdle;
const TapState kFTB = ImmediateInterpreter::kTtcFirstTapBegan;
const TapState kTpC = ImmediateInterpreter::kTtcTapComplete;
const TapState kSTB = ImmediateInterpreter::kTtcSubsequentTapBegan;
const TapState kDrg = ImmediateInterpreter::kTtcDrag;
const TapState kDRl = ImmediateInterpreter::kTtcDragRelease;
const TapState kDRt = ImmediateInterpreter::kTtcDragRetouch;
const unsigned kBL = GESTURES_BUTTON_LEFT;
const unsigned kBM = GESTURES_BUTTON_MIDDLE;
const unsigned kBR = GESTURES_BUTTON_RIGHT;

TEST(ImmediateInterpreterTest, TapToClickStateMachineTest) {
  std::unique_ptr<ImmediateInterpreter> ii;

  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    200,  // right edge
    200,  // bottom edge
    1.0,  // pixels/TP width
    1.0,  // pixels/TP height
    1.0,  // screen DPI x
    1.0,  // screen DPI y
    -1,  // orientation minimum
    2,   // orientation maximum
    5,  // max fingers
    5,  // max touch
    0,  // t5r2
    0,  // semi-mt
    1,  // is button pad
    0   // has_wheel
  };
  TestInterpreterWrapper wrapper(ii.get(), &hwprops);

  FingerState fs[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID
    {0, 0, 0, 0, 50, 0, 4, 4, 91, 0},
    {0, 0, 0, 0, 75, 0, 4, 9, 92, 0},
    {0, 0, 0, 0, 50, 0, 4, 4, 93, 0},
    {0, 0, 0, 0, 50, 0, 4, 4, 94, 0},
    {0, 0, 0, 0, 50, 0, 4, 4, 95, 0},
    {0, 0, 0, 0, 50, 0, 6, 4, 95, 0},
    {0, 0, 0, 0, 50, 0, 8, 4, 95, 0},
    {0, 0, 0, 0, 50, 0, 4, 4, 96, 0},
    {0, 0, 0, 0, 50, 0, 6, 4, 96, 0},
    {0, 0, 0, 0, 50, 0, 8, 4, 96, 0},

    {0, 0, 0, 0, 50, 0, 4, 1, 97, 0},  // 10
    {0, 0, 0, 0, 50, 0, 9, 1, 98, 0},
    {0, 0, 0, 0, 50, 0, 4, 5, 97, 0},
    {0, 0, 0, 0, 50, 0, 9, 5, 98, 0},
    {0, 0, 0, 0, 50, 0, 4, 9, 97, 0},
    {0, 0, 0, 0, 50, 0, 9, 9, 98, 0},

    {0, 0, 0, 0, 80, 0, 5, 9, 70, 0},  // 16; thumb
    {0, 0, 0, 0, 50, 0, 4, 4, 91, 0},
    {0, 0, 0, 0, 80, 0, 5, 9, 71, 0},  // thumb-new id

    {0, 0, 0, 0, 50, 0, 8.0, 4, 95, 0},  // 19; very close together fingers:
    {0, 0, 0, 0, 50, 0, 8.1, 4, 96, 0},
    {0, 0, 0, 0, 15, 0, 9.5, 4, 95, 0},  // very light pressure
    {0, 0, 0, 0, 15, 0, 11,  4, 96, 0},

    {0, 0, 0, 0, 50, 0, 20, 4, 95, 0},  // 23; Two fingers very far apart
    {0, 0, 0, 0, 50, 0, 90, 4, 96, 0},

    {0, 0, 0, 0, 50, 0, 50, 40, 95, 0},  // 25
    {0, 0, 0, 0, 15, 0, 70, 40, 96, 0},  // very light pressure

    {0, 0, 0, 0, 50, 0, 4, 1, 97, 0},  // 27
    {0, 0, 0, 0,  3, 0, 9, 1, 98, 0},

    {0, 0, 0, 0, 50, 0,  4, 1, 97, 0},  // 29
    {0, 0, 0, 0, 50, 0,  9, 1, 98, 0},
    {0, 0, 0, 0, 50, 0, 14, 1, 99, 0},

    {0, 0, 0, 0, 50, 0, 50, 40, 95, 0},  // 32
    {0, 0, 0, 0, 50, 0, 70, 40, 96, GESTURES_FINGER_NO_TAP},

    {0, 0, 0, 0, 50, 0, 4, 4, 91, GESTURES_FINGER_PALM},  // 34
  };
  HWStateGs hwsgs[] = {
    // Simple 1-finger tap
    {S,{0.00,0,1,1,&fs[0],0,0,0,0},-1,MkSet(91),0,0,kFTB,false},
    {C,{0.01,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kTpC,true},
    {C,{0.07,0,0,0,NULL,0,0,0,0},.07,MkSet(),kBL,kBL,kIdl,false},
    // Simple 1-finger tap w/o dragging enabled
    {N,{0.00,0,1,1,&fs[0],0,0,0,0},-1,MkSet(91),0,0,kFTB,false},
    {C,{0.01,0,0,0,NULL,0,0,0,0},-1,MkSet(),kBL,kBL,kIdl,false},
    // 1-finger tap with click
    {S,{0.00,kBL,1,1,&fs[0],0,0,0,0},-1,MkSet(91),0,0,kIdl,false},
    {C,{0.01,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kIdl,false},
    {C,{0.07,0,0,0,NULL,0,0,0,0},.07,MkSet(),0,0,kIdl,false},
    // 1-finger swipe
    {S,{0.00,0,1,1,&fs[4],0,0,0,0},-1,MkSet(95),0,0,kFTB,false},
    {C,{0.01,0,1,1,&fs[5],0,0,0,0},-1,MkSet(95),0,0,kIdl,false},
    {C,{0.02,0,1,1,&fs[6],0,0,0,0},-1,MkSet(95),0,0,kIdl,false},
    {C,{0.03,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kIdl,false},
    // Double 1-finger tap
    {S,{0.00,0,1,1,&fs[0],0,0,0,0},-1,MkSet(91),0,0,kFTB,false},
    {C,{0.01,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kTpC,true},
    {C,{0.02,0,1,1,&fs[2],0,0,0,0},-1,MkSet(93),0,0,kSTB,false},
    {C,{0.03,0,0,0,NULL,0,0,0,0},-1,MkSet(),kBL,kBL,kTpC,true},
    {C,{0.09,0,0,0,NULL,0,0,0,0},.09,MkSet(),kBL,kBL,kIdl,false},
    // Triple 1-finger tap
    {S,{0.00,0,1,1,&fs[0],0,0,0,0},-1,MkSet(91),0,0,kFTB,false},
    {C,{0.01,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kTpC,true},
    {C,{0.02,0,1,1,&fs[2],0,0,0,0},-1,MkSet(93),0,0,kSTB,false},
    {C,{0.03,0,0,0,NULL,0,0,0,0},-1,MkSet(),kBL,kBL,kTpC,true},
    {C,{0.04,0,1,1,&fs[3],0,0,0,0},-1,MkSet(94),0,0,kSTB,false},
    {C,{0.05,0,0,0,NULL,0,0,0,0},-1,MkSet(),kBL,kBL,kTpC,true},
    {C,{0.11,0,0,0,NULL,0,0,0,0},.11,MkSet(),kBL,kBL,kIdl,false},
    // 1-finger tap + drag
    {S,{0.00,0,1,1,&fs[0],0,0,0,0},-1,MkSet(91),0,0,kFTB,false},
    {C,{0.01,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kTpC,true},
    {C,{0.02,0,1,1,&fs[4],0,0,0,0},-1,MkSet(95),0,0,kSTB,false},
    {C,{0.13,0,1,1,&fs[5],0,0,0,0},-1,MkSet(95),kBL,0,kDrg,false},
    {C,{0.14,0,1,1,&fs[6],0,0,0,0},-1,MkSet(95),0,0,kDrg,false},
    {C,{0.15,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kDRl,true},
    {C,{0.99,0,0,0,NULL,0,0,0,0},.99,MkSet(),0,kBL,kIdl,false},
    // 1-finger tap + move
    {S,{0.00,0,1,1,&fs[0],0,0,0,0},-1,MkSet(91),0,0,kFTB,false},
    {C,{0.01,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kTpC,true},
    {C,{0.02,0,1,1,&fs[4],0,0,0,0},-1,MkSet(95),0,0,kSTB,false},
    {C,{0.03,0,1,1,&fs[5],0,0,0,0},-1,MkSet(95),kBL,kBL,kIdl,false},
    {C,{0.04,0,1,1,&fs[6],0,0,0,0},-1,MkSet(95),0,0,kIdl,false},
    {C,{0.05,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kIdl,false},
    // 1-finger tap, move, release, move again (drag lock)
    {S,{0.00,0,1,1,&fs[0],0,0,0,0},-1,MkSet(91),0,0,kFTB,false},
    {C,{0.01,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kTpC,true},
    {C,{0.02,0,1,1,&fs[4],0,0,0,0},-1,MkSet(95),0,0,kSTB,false},
    {C,{0.08,0,1,1,&fs[5],0,0,0,0},-1,MkSet(95),kBL,0,kDrg,false},
    {C,{0.09,0,1,1,&fs[6],0,0,0,0},-1,MkSet(95),0,0,kDrg,false},
    {C,{0.10,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kDRl,true},
    {C,{0.11,0,1,1,&fs[7],0,0,0,0},-1,MkSet(96),0,0,kDRt,false},
    {C,{0.12,0,1,1,&fs[8],0,0,0,0},-1,MkSet(96),0,0,kDrg,false},
    {C,{0.13,0,1,1,&fs[9],0,0,0,0},-1,MkSet(96),0,0,kDrg,false},
    {C,{0.14,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kDRl,true},
    {C,{0.99,0,0,0,NULL,0,0,0,0},.99,MkSet(),0,kBL,kIdl,false},
    // 1-finger long press
    {S,{0.00,0,1,1,&fs[0],0,0,0,0},-1,MkSet(91),0,0,kFTB,false},
    {C,{0.02,0,1,1,&fs[0],0,0,0,0},-1,MkSet(91),0,0,kFTB,false},
    {C,{0.04,0,1,1,&fs[0],0,0,0,0},-1,MkSet(91),0,0,kFTB,false},
    {C,{0.06,0,1,1,&fs[0],0,0,0,0},-1,MkSet(91),0,0,kIdl,false},
    {C,{0.07,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kIdl,false},
    // 1-finger tap then long press
    {S,{0.00,0,1,1,&fs[0],0,0,0,0},-1,MkSet(91),0,0,kFTB,false},
    {C,{0.01,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kTpC,true},
    {C,{0.02,0,1,1,&fs[4],0,0,0,0},-1,MkSet(95),0,0,kSTB,false},
    {C,{0.14,0,1,1,&fs[4],0,0,0,0},-1,MkSet(95),kBL,0,kDrg,false},
    {C,{0.16,0,1,1,&fs[4],0,0,0,0},-1,MkSet(95),0,0,kDrg,false},
    {C,{0.18,0,1,1,&fs[4],0,0,0,0},-1,MkSet(95),0,0,kDrg,false},
    {C,{0.19,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kDRl,true},
    {C,{0.99,0,0,0,NULL,0,0,0,0},.99,MkSet(),0,kBL,kIdl,false},
    // 2-finger tap (right click)
    {S,{0.00,0,2,2,&fs[10],0,0,0,0},-1,MkSet(97,98),0,0,kFTB,false},
    {C,{0.01,0,0,0,NULL,0,0,0,0},-1,MkSet(),kBR,kBR,kIdl,false},
    {C,{0.07,0,0,0,NULL,0,0,0,0},.07,MkSet(),0,0,kIdl,false},
    // 3-finger tap (middle click)
    {S,{0.00,0,3,3,&fs[29],0,0,0,0},-1,MkSet(97,98,99),0,0,kFTB,false},
    {C,{0.01,0,0,0,NULL,0,0,0,0},-1,MkSet(),kBM,kBM,kIdl,false},
    {C,{0.07,0,0,0,NULL,0,0,0,0},.07,MkSet(),0,0,kIdl,false},
    // 2-finger tap, but one finger is very very light, so left tap
    {S,{0.00,0,2,2,&fs[27],0,0,0,0},-1,MkSet(97,98),0,0,kFTB,false},
    {C,{0.01,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kTpC,true},
    {C,{0.07,0,0,0,NULL,0,0,0,0},.07,MkSet(),kBL,kBL,kIdl,false},
    // 2-finger scroll
    {S,{0.00,0,2,2,&fs[10],0,0,0,0},-1,MkSet(97,98),0,0,kFTB,false},
    {C,{0.01,0,2,2,&fs[12],0,0,0,0},-1,MkSet(97,98),0,0,kIdl,false},
    {C,{0.02,0,2,2,&fs[14],0,0,0,0},-1,MkSet(97,98),0,0,kIdl,false},
    {C,{0.03,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kIdl,false},
    // left tap, right tap
    {S,{0.00,0,1,1,&fs[0],0,0,0,0},-1,MkSet(91),0,0,kFTB,false},
    {C,{0.01,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kTpC,true},
    {C,{0.02,0,2,2,&fs[10],0,0,0,0},-1,MkSet(97,98),kBL,kBL,kFTB,false},
    {C,{0.03,0,0,0,NULL,0,0,0,0},-1,MkSet(),kBR,kBR,kIdl,false},
    {C,{0.09,0,0,0,NULL,0,0,0,0},.09,MkSet(),0,0,kIdl,false},
    // left tap, multi-frame right tap
    {S,{0.00,0,1,1,&fs[0],0,0,0,0},-1,MkSet(91),0,0,kFTB,false},
    {C,{0.01,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kTpC,true},
    {C,{0.02,0,1,1,&fs[10],0,0,0,0},-1,MkSet(97),0,0,kSTB,false},
    {C,{0.03,0,1,1,&fs[11],0,0,0,0},-1,MkSet(98),kBL,kBL,kFTB,false},
    {C,{0.04,0,0,0,NULL,0,0,0,0},-1,MkSet(),kBR,kBR,kIdl,false},
    // right tap, left tap
    {S,{0.00,0,2,2,&fs[10],0,0,0,0},-1,MkSet(97,98),0,0,kFTB,false},
    {C,{0.01,0,0,0,NULL,0,0,0,0},-1,MkSet(),kBR,kBR,kIdl,false},
    {C,{0.02,0,1,1,&fs[0],0,0,0,0},-1,MkSet(91),0,0,kFTB,false},
    {C,{0.03,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kTpC,true},
    {C,{0.09,0,0,0,NULL,0,0,0,0},.09,MkSet(),kBL,kBL,kIdl,false},
    // middle tap, left tap
    {S,{0.00,0,3,3,&fs[29],0,0,0,0},-1,MkSet(97,98,99),0,0,kFTB,false},
    {C,{0.01,0,0,0,NULL,0,0,0,0},-1,MkSet(),kBM,kBM,kIdl,false},
    {C,{0.02,0,1,1,&fs[0],0,0,0,0},-1,MkSet(91),0,0,kFTB,false},
    {C,{0.03,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kTpC,true},
    {C,{0.09,0,0,0,NULL,0,0,0,0},.09,MkSet(),kBL,kBL,kIdl,false},
    // right double-tap
    {S,{0.00,0,2,2,&fs[6],0,0,0,0},-1,MkSet(95,96),0,0,kFTB,false},
    {C,{0.01,0,0,0,NULL,0,0,0,0},-1,MkSet(),kBR,kBR,kIdl,false},
    {C,{0.02,0,2,2,&fs[10],0,0,0,0},-1,MkSet(97,98),0,0,kFTB,false},
    {C,{0.03,0,0,0,NULL,0,0,0,0},-1,MkSet(),kBR,kBR,kIdl,false},
    {C,{0.09,0,0,0,NULL,0,0,0,0},.09,MkSet(),0,0,kIdl,false},
    // left drumroll separation on fast swipe
    {S,{0.00,0,1,1,&fs[32],0,0,0,0},-1,MkSet(95),0,0,kFTB,false},
    {C,{0.01,0,1,1,&fs[33],0,0,0,0},-1,MkSet(96),0,0,kIdl,false},
    {C,{0.02,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kIdl,false},
    // left tap, right-drag
    {S,{0.00,0,1,1,&fs[0],0,0,0,0},-1,MkSet(91),0,0,kFTB,false},
    {C,{0.01,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kTpC,true},
    {C,{0.02,0,2,2,&fs[10],0,0,0,0},-1,MkSet(97,98),kBL,kBL,kFTB,false},
    {C,{0.03,0,2,2,&fs[12],0,0,0,0},-1,MkSet(97,98),0,0,kIdl,false},
    {C,{0.04,0,2,2,&fs[14],0,0,0,0},-1,MkSet(97,98),0,0,kIdl,false},
    {C,{0.05,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kIdl,false},
    // left tap, right multi-frame drag
    {S,{0.00,0,1,1,&fs[0],0,0,0,0},-1,MkSet(91),0,0,kFTB,false},
    {C,{0.01,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kTpC,true},
    {C,{0.02,0,1,1,&fs[10],0,0,0,0},-1,MkSet(97),0,0,kSTB,false},
    {C,{0.03,0,2,2,&fs[12],0,0,0,0},-1,MkSet(97,98),kBL,kBL,kIdl,false},
    {C,{0.04,0,2,2,&fs[14],0,0,0,0},-1,MkSet(97,98),0,0,kIdl,false},
    {C,{0.05,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kIdl,false},
    // left tap, drag + finger joined later
    {S,{0.00,0,1,1,&fs[0],0,0,0,0},-1,MkSet(91),0,0,kFTB,false},
    {C,{0.01,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kTpC,true},
    {C,{0.02,0,1,1,&fs[10],0,0,0,0},-1,MkSet(97),0,0,kSTB,false},
    {C,{0.13,0,1,1,&fs[10],0,0,0,0},-1,MkSet(97),kBL,0,kDrg,false},
    {C,{0.14,0,2,2,&fs[12],0,0,0,0},-1,MkSet(97,98),0,kBL,kIdl,false},
    {C,{0.15,0,2,2,&fs[14],0,0,0,0},-1,MkSet(97,98),0,0,kIdl,false},
    {C,{0.16,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kIdl,false},
    // right tap, left-drag
    {S,{0.00,0,2,2,&fs[14],0,0,0,0},-1,MkSet(97,98),0,0,kFTB,false},
    {C,{0.01,0,0,0,NULL,0,0,0,0},-1,MkSet(),kBR,kBR,kIdl,false},
    {C,{0.02,0,1,1,&fs[4],0,0,0,0},-1,MkSet(95),0,0,kFTB,false},
    {C,{0.03,0,1,1,&fs[5],0,0,0,0},-1,MkSet(95),0,0,kIdl,false},
    {C,{0.04,0,1,1,&fs[6],0,0,0,0},-1,MkSet(95),0,0,kIdl,false},
    {C,{0.05,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kIdl,false},
    // right tap, right-drag
    {S,{0.00,0,2,2,&fs[6],0,0,0,0},-1,MkSet(95,96),0,0,kFTB,false},
    {C,{0.01,0,0,0,NULL,0,0,0,0},-1,MkSet(),kBR,kBR,kIdl,false},
    {C,{0.02,0,2,2,&fs[10],0,0,0,0},-1,MkSet(97,98),0,0,kFTB,false},
    {C,{0.03,0,2,2,&fs[12],0,0,0,0},-1,MkSet(97,98),0,0,kIdl,false},
    {C,{0.04,0,2,2,&fs[14],0,0,0,0},-1,MkSet(97,98),0,0,kIdl,false},
    {C,{0.05,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kIdl,false},
    // drag then right-tap
    {S,{0.00,0,1,1,&fs[0],0,0,0,0},-1,MkSet(91),0,0,kFTB,false},
    {C,{0.01,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kTpC,true},
    {C,{0.02,0,1,1,&fs[4],0,0,0,0},-1,MkSet(95),0,0,kSTB,false},
    {C,{0.10,0,1,1,&fs[5],0,0,0,0},-1,MkSet(95),kBL,0,kDrg,false},
    {C,{0.11,0,1,1,&fs[6],0,0,0,0},-1,MkSet(95),0,0,kDrg,false},
    {C,{0.12,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kDRl,true},
    {C,{0.13,0,2,2,&fs[10],0,0,0,0},-1,MkSet(97,98),0,0,kDRt,false},
    {C,{0.14,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,kBL,kTpC,true},
    {C,{0.99,0,0,0,NULL,0,0,0,0},.99,MkSet(),kBR,kBR,kIdl,false},
    // slow double tap
    {D,{ 0.00, 0, 1, 1, &fs[0],0,0,0,0}, -1, MkSet(91),   0,   0, kFTB, false },
    {C,{0.10,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kTpC,true},
    {C,{0.12,0,1,1,&fs[2],0,0,0,0},-1,MkSet(93),0,0,kSTB,false},
    {C,{0.22,0,0,0,NULL,0,0,0,0},-1,MkSet(),kBL,kBL,kTpC,true},
    {C,{0.90,0,0,0,NULL,0,0,0,0},.9,MkSet(),kBL,kBL,kIdl,false},
    // right tap, very close fingers - shouldn't tap
    {S,{0.00,0,2,2,&fs[19],0,0,0,0},-1,MkSet(95,96),0,0,kIdl,false},
    {C,{0.01,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kIdl,false},
    // very light left tap - shouldn't tap
    {S,{0.00,0,1,1,&fs[21],0,0,0,0},-1,MkSet(95),0,0,kFTB,false},
    {C,{0.01,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kIdl,false},
    // very light right tap - shouldn't tap
    {S,{0.00,0,2,2,&fs[21],0,0,0,0},-1,MkSet(95,96),0,0,kFTB,false},
    {C,{0.01,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kIdl,false},
    // half very light right tap - should tap
    {S,{0.00,0,2,2,&fs[20],0,0,0,0},-1,MkSet(95,96),0,0,kFTB,false},
    {C,{0.01,0,0,0,NULL,0,0,0,0},-1,MkSet(),kBR,kBR,kIdl,false},
    // Right tap, w/ fingers too far apart - shouldn't right tap
    {S,{0.00,0,2,2,&fs[23],0,0,0,0},-1,MkSet(95,96),0,0,kFTB,false},
    {C,{0.01,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kTpC,true},
    {C,{0.07,0,0,0,NULL,0,0,0,0},.07,MkSet(),kBL,kBL,kIdl,false},
    // Two fingers merge into one, then leave - shouldn't tap
    {S,{0.00,0,2,2,&fs[6],0,0,0,0},-1,MkSet(95,96),0,0,kFTB,false},
    {C,{1.00,0,2,2,&fs[6],0,0,0,0},-1,MkSet(95,96),0,0,kIdl,false},
    {C,{1.01,0,1,1,&fs[17],0,0,0,0},-1,MkSet(91),0,0,kIdl,false},
    {C,{1.02,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kIdl,false},
    // 1-finger marked as palm for a long time then unmarked - shouldn't tap
    {S,{0.00,0,1,1,&fs[34],0,0,0,0},-1,MkSet(),0,0,kIdl,false},
    {C,{1.50,0,1,1,&fs[0],0,0,0,0},-1,MkSet(91),0,0,kIdl,false},
    {C,{1.51,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kIdl,false},

    //{C,{ 0.08, 0, 0, 0, NULL, 0, 0, 0, 0 }, .07, MkSet(), 0, 0, kIdl, false },
    // Two fingers seem to tap, the bigger of which is the only one that
    // meets the minimum pressure threshold. Then that higher pressure finger
    // is no longer gesturing (e.g., it gets classified as a thumb).
    // There should be no tap b/c the one remaining finger didn't meet the
    // minimum pressure threshold.
    {S,{0.00,0,2,2,&fs[25],0,0,0,0},-1,MkSet(95,96),0,0,kFTB,false},
    {C,{0.01,0,2,2,&fs[25],0,0,0,0},-1,MkSet(96),0,0,kFTB,false},
    {C,{0.02,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kIdl,false},
    // 2f click - shouldn't tap
    {S,{0.00,0,2,2,&fs[0],0,0,0,0},-1,MkSet(91,92),0,0,kFTB,false},
    {C,{0.01,1,2,2,&fs[0],0,0,0,0},-1,MkSet(91,92),0,0,kIdl,false},
    {C,{0.02,0,2,2,&fs[0],0,0,0,0},-1,MkSet(91,92),0,0,kIdl,false},
    {C,{0.03,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kIdl,false},
    // T5R2 tap tests:
    // (1f and 2f tap w/o resting thumb and 1f w/ resting thumb are the same as
    // above)
    // 2f tap w/ resting thumb
    {T,{ 0.00,0,1,1,&fs[16],0,0,0,0 },-1,MkSet(70),0,0,kFTB,false },
    {C,{1.00,0,1,1,&fs[16],0,0,0,0},-1,MkSet(70),0,0,kIdl,false},
    {C,{1.01,0,1,3,&fs[16],0,0,0,0},-1,MkSet(70),0,0,kFTB,false},
    {C,{1.02,0,2,3,&fs[16],0,0,0,0},-1,MkSet(70,91),0,0,kFTB,false},
    {C,{1.03,0,0,2,NULL,0,0,0,0},-1,MkSet(),0,0,kFTB,false},
    {C,{1.04,0,1,1,&fs[18],0,0,0,0},-1,MkSet(71),kBR,kBR,kIdl,false},
    // 3f tap w/o resting thumb
    {S,{0.00,0,2,3,&fs[0],0,0,0,0},-1,MkSet(91,92),0,0,kFTB,false},
    {C,{0.01,0,0,1,NULL,0,0,0,0},-1,MkSet(),0,0,kFTB,false},
    {C,{0.02,0,0,0,NULL,0,0,0,0},-1,MkSet(),kBM,kBM,kIdl,false},
    // 3f tap w/o resting thumb (slightly different)
    {S,{0.00,0,2,3,&fs[0],0,0,0,0},-1,MkSet(91,92),0,0,kFTB,false},
    {C,{0.01,0,2,3,&fs[0],0,0,0,0},-1,MkSet(91,92),0,0,kFTB,false},
    {C,{0.02,0,0,0,NULL,0,0,0,0},-1,MkSet(),kBM,kBM,kIdl,false},
    // 3f tap w/ resting thumb
    {S,{0.00,0,1,1,&fs[16],0,0,0,0},-1,MkSet(70),0,0,kFTB,false},
    {C,{1.00,0,1,1,&fs[16],0,0,0,0},-1,MkSet(70),0,0,kIdl,false},
    {C,{1.01,0,1,4,&fs[16],0,0,0,0},-1,MkSet(70),0,0,kFTB,false},
    {C,{1.02,0,2,4,&fs[16],0,0,0,0},-1,MkSet(70,91),0,0,kFTB,false},
    {C,{1.03,0,1,1,&fs[16],0,0,0,0},-1,MkSet(70),kBM,kBM,kIdl,false},
    // 4f tap w/o resting thumb
    {S,{0.00,0,2,3,&fs[0],0,0,0,0},-1,MkSet(91,92),0,0,kFTB,false},
    {C,{0.01,0,1,4,&fs[0],0,0,0,0},-1,MkSet(91),0,0,kFTB,false},
    {C,{0.02,0,2,4,&fs[0],0,0,0,0},-1,MkSet(91,92),0,0,kFTB,false},
    {C,{0.03,0,0,0,NULL,0,0,0,0},-1,MkSet(),kBR,kBR,kIdl,false},
    // 4f tap w/ resting thumb
    {S,{0.00,0,1,1,&fs[16],0,0,0,0},-1,MkSet(70),0,0,kFTB,false},
    {C,{1.00,0,1,1,&fs[16],0,0,0,0},-1,MkSet(70),0,0,kIdl,false},
    {C,{1.01,0,1,5,&fs[16],0,0,0,0},-1,MkSet(70),0,0,kFTB,false},
    {C,{1.02,0,1,1,&fs[16],0,0,0,0},-1,MkSet(70),kBR,kBR,kIdl,false},
    // 4f tap w/ resting thumb (slightly different)
    {S,{0.00,0,1,1,&fs[16],0,0,0,0},-1,MkSet(70),0,0,kFTB,false},
    {C,{1.00,0,1,1,&fs[16],0,0,0,0},-1,MkSet(70),0,0,kIdl,false},
    {C,{1.01,0,1,5,&fs[16],0,0,0,0},-1,MkSet(70),0,0,kFTB,false},
    {C,{1.02,0,2,5,&fs[16],0,0,0,0},-1,MkSet(70,91),0,0,kFTB,false},
    {C,{1.03,0,1,1,&fs[16],0,0,0,0},-1,MkSet(70),kBR,kBR,kIdl,false},
    // 3f letting go, shouldn't tap at all
    {S,{0.00,0,2,3,&fs[0],0,0,0,0},-1,MkSet(91,92),0,0,kFTB,false},
    {C,{1.01,0,2,3,&fs[0],0,0,0,0},-1,MkSet(91,92),0,0,kIdl,false},
    {C,{1.02,0,0,2,NULL,0,0,0,0},-1,MkSet(),0,0,kIdl,false},
    {C,{1.03,0,2,2,&fs[10],0,0,0,0},-1,MkSet(97,98),0,0,kIdl,false},
    {C,{1.04,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kIdl,false},
    // tap then move. no drag expected
    {S,{0.00,0,1,1,&fs[0],0,0,0,0},-1,MkSet(91),0,0,kFTB,false},
    {C,{0.01,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kTpC,true},
    {C,{0.02,0,1,1,&fs[4],0,0,0,0},-1,MkSet(95),0,0,kSTB,false},
    {C,{0.03,0,1,1,&fs[5],0,0,0,0},-1,MkSet(95),kBL,kBL,kIdl,false},
    {C,{0.05,0,1,1,&fs[6],0,0,0,0},-1,MkSet(95),0,0,kIdl,false},
    {C,{0.06,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kIdl,false},
    {C,{0.99,0,0,0,NULL,0,0,0,0},.99,MkSet(),0,0,kIdl,false},
  };
  const size_t kT5R2TestFirstIndex = NonT5R2States(hwsgs, arraysize(hwsgs));

  // Algorithmically add a resting thumb to a copy of all above cases
  const size_t hwsgs_full_size = arraysize(hwsgs) + kT5R2TestFirstIndex;
  std::vector<HWStateGs> hwsgs_full;
  hwsgs_full.reserve(hwsgs_full_size);
  std::copy(hwsgs, hwsgs + arraysize(hwsgs), hwsgs_full.begin());
  std::copy(hwsgs, hwsgs + kT5R2TestFirstIndex,
            hwsgs_full.begin() + arraysize(hwsgs));

  std::vector<std::vector<FingerState> > thumb_fs(arraysize(hwsgs));
  const FingerState& fs_thumb = fs[18];
  bool thumb_gestures = true;
  for (size_t i = 0; i < kT5R2TestFirstIndex; ++i) {
    HardwareState* hs = &hwsgs_full[i + arraysize(hwsgs)].hws;
    if (hwsgs_full[i].IsStart())
      thumb_gestures = true;  // Start out w/ thumb being able to gesture
    if (hs->finger_cnt > 0)
      thumb_gestures = false;  // Once a finger is present, thumb can't gesture
    std::vector<FingerState>& newfs = thumb_fs[i];
    newfs.resize(hs->finger_cnt + 1);
    newfs[0] = fs_thumb;
    for (size_t j = 0; j < hs->finger_cnt; ++j)
      newfs[j + 1] = hs->fingers[j];
    set<short, kMaxGesturingFingers>& gs = hwsgs_full[i + arraysize(hwsgs)].gs;
    if (thumb_gestures)
      gs.insert(fs_thumb.tracking_id);
    hs->fingers = &thumb_fs[i][0];
    hs->finger_cnt++;
    hs->touch_cnt++;
  }

  for (size_t i = 0; i < hwsgs_full_size; ++i) {
    string desc;
    if (i < arraysize(hwsgs))
      desc = StringPrintf("State %zu, Line Number %ld", i,
                          hwsgs[i].LineNumber());
    else
      desc = StringPrintf("State %zu (resting thumb), Line Number %ld",
                          i - arraysize(hwsgs),
                          hwsgs[i - arraysize(hwsgs)].LineNumber());

    unsigned bdown = 0;
    unsigned bup = 0;
    stime_t tm = -1.0;
    bool same_fingers = false;
    HardwareState* hwstate = &hwsgs_full[i].hws;
    stime_t now = hwsgs_full[i].callback_now;
    if (hwsgs_full[i].callback_now >= 0.0) {
      hwstate = NULL;
    } else {
      now = hwsgs_full[i].hws.timestamp;
    }

    if (hwstate && hwstate->timestamp == 0.0) {
      // Reset imm interpreter
      fprintf(stderr, "Resetting imm interpreter, i = %zd\n", i);
      ii.reset(new ImmediateInterpreter(NULL, NULL));
      wrapper.Reset(ii.get());
      ii->drag_lock_enable_.val_ = 1;
      ii->motion_tap_prevent_timeout_.val_ = 0;
      ii->tapping_finger_min_separation_.val_ = 1.0;
      ii->tap_drag_timeout_.val_ = 0.05;
      ii->tap_enable_.val_ = 1;
      ii->tap_drag_enable_.val_ =
          !(hwsgs_full[i].line_number_and_flags & HWStateFlagStartNoDrag);
      ii->tap_move_dist_.val_ = 1.0;
      ii->tap_timeout_.val_ = ii->inter_tap_timeout_.val_ = 0.05;
      ii->three_finger_click_enable_.val_ = 1;
      ii->t5r2_three_finger_click_enable_.val_ = 1;
      // For the slow tap case, we need to make tap_timeout_ bigger
      if (hwsgs_full[i].line_number_and_flags & HWStateFlagStartDoubleTap)
        ii->tap_timeout_.val_ = 0.11;
      EXPECT_EQ(kIdl, ii->tap_to_click_state_);
    } else {
      same_fingers = ii->state_buffer_.Get(1)->SameFingersAs(hwsgs_full[i].hws);
    }

    if (hwstate)
      ii->state_buffer_.PushState(*hwstate);
    ii->UpdateTapState(
        hwstate, hwsgs_full[i].gs, same_fingers, now, &bdown, &bup, &tm);
    ii->prev_gs_fingers_ = hwsgs_full[i].gs;
    EXPECT_EQ(hwsgs_full[i].expected_down, bdown) << desc;
    EXPECT_EQ(hwsgs_full[i].expected_up, bup) << desc;
    if (hwsgs_full[i].timeout)
      EXPECT_GT(tm, 0.0) << desc;
    else
      EXPECT_DOUBLE_EQ(-1.0, tm) << desc;
    EXPECT_EQ(hwsgs_full[i].expected_state, ii->tap_to_click_state_)
        << desc;
  }
}

namespace {

struct TapToClickLowPressureBeginOrEndInputs {
  stime_t now;
  float x0, y0, p0;  // (x, y), pressure
  short id0;  // tracking id
  float x1, y1, p1;  // (x, y), pressure
  short id1;  // tracking id
};

}  // namespace {}

// Test that if a tap contact has some frames before and after that tap, with
// a finger that's located far from the tap spot, but has low pressure at that
// location, it's still a tap. We see this happen on some hardware particularly
// for right clicks. This is based on two logs from Ken Moore.
TEST(ImmediateInterpreterTest, TapToClickLowPressureBeginOrEndTest) {
  std::unique_ptr<ImmediateInterpreter> ii;
  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    106.666672,  // right edge
    68.000000,  // bottom edge
    1,  // pixels/TP width
    1,  // pixels/TP height
    25.4,  // screen DPI x
    25.4,  // screen DPI y
    -1,  // orientation minimum
    2,   // orientation maximum
    15,  // max fingers
    5,  // max touch
    0,  // t5r2
    0,  // semi-mt
    true,  // is button pad
    0   // has_wheel
  };
  TestInterpreterWrapper wrapper(ii.get(), &hwprops);

  TapToClickLowPressureBeginOrEndInputs inputs[] = {
    // Two runs
    { 32.4901, 55.5, 24.8,  7.0,  1,  0.0,  0.0,  0.0, -1 },
    { 32.5010, 57.7, 25.0, 43.0,  1, 42.0, 27.5, 36.0,  2 },
    { 32.5118, 58.0, 25.0, 44.0,  1, 42.0, 27.5, 43.0,  2 },
    { 32.5227, 58.0, 25.0, 44.0,  1, 42.0, 27.6, 44.0,  2 },
    { 32.5335, 58.0, 25.0, 45.0,  1, 42.0, 27.6, 45.0,  2 },
    { 32.5443, 58.0, 25.0, 44.0,  1, 42.0, 27.6, 45.0,  2 },
    { 32.5552, 58.0, 25.0, 44.0,  1, 42.0, 27.6, 44.0,  2 },
    { 32.5661, 58.0, 25.0, 42.0,  1, 42.0, 27.6, 42.0,  2 },
    { 32.5769, 58.0, 25.0, 35.0,  1, 42.0, 27.6, 35.0,  2 },
    { 32.5878, 58.0, 25.0, 15.0,  1, 41.9, 27.6, 18.0,  2 },
    { 32.5965, 45.9, 27.5,  7.0,  2,  0.0,  0.0,  0.0, -1 },
    { 32.6042,  0.0,  0.0,  0.0, -1,  0.0,  0.0,  0.0, -1 },

    { 90.6057, 64.0, 37.0, 18.0,  1,  0.0,  0.0,  0.0, -1 },
    { 90.6144, 63.6, 37.0, 43.0,  1,  0.0,  0.0,  0.0, -1 },
    { 90.6254, 63.6, 37.0, 43.0,  1, 46.5, 40.2, 47.0,  2 },
    { 90.6361, 63.6, 37.0, 44.0,  1, 46.5, 40.2, 44.0,  2 },
    { 90.6470, 63.6, 37.0, 45.0,  1, 46.5, 40.2, 46.0,  2 },
    { 90.6579, 63.6, 37.0, 45.0,  1, 46.5, 40.2, 46.0,  2 },
    { 90.6686, 63.6, 37.0, 45.0,  1, 46.5, 40.2, 47.0,  2 },
    { 90.6795, 63.6, 37.0, 46.0,  1, 46.5, 40.2, 47.0,  2 },
    { 90.6903, 63.6, 37.0, 45.0,  1, 46.5, 40.2, 46.0,  2 },
    { 90.7012, 63.6, 37.0, 44.0,  1, 46.3, 40.2, 44.0,  2 },
    { 90.7121, 63.6, 37.2, 38.0,  1, 46.4, 40.2, 31.0,  2 },
    { 90.7229, 63.6, 37.4, 22.0,  1, 46.4, 40.2, 12.0,  2 },
    { 90.7317, 61.1, 38.0,  8.0,  1,  0.0,  0.0,  0.0, -1 },
    { 90.7391,  0.0,  0.0,  0.0, -1,  0.0,  0.0,  0.0, -1 },
  };

  bool reset_next_time = true;
  for (size_t i = 0; i < arraysize(inputs); i++) {
    const TapToClickLowPressureBeginOrEndInputs& input = inputs[i];
    if (reset_next_time) {
      ii.reset(new ImmediateInterpreter(NULL, NULL));
      wrapper.Reset(ii.get());
      ii->tap_enable_.val_ = 1;
      reset_next_time = false;
    }
    // Prep inputs
    FingerState fs[] = {
      { 0, 0, 0, 0, input.p0, 0, input.x0, input.y0, input.id0, 0 },
      { 0, 0, 0, 0, input.p1, 0, input.x1, input.y1, input.id1, 0 },
    };
    unsigned short finger_cnt = fs[0].tracking_id == -1 ? 0 :
        (fs[1].tracking_id == -1 ? 1 : 2);
    HardwareState hs = { input.now, 0, finger_cnt, finger_cnt, fs, 0, 0, 0, 0 };
    stime_t timeout = -1;
    Gesture* gs = wrapper.SyncInterpret(&hs, &timeout);
    if (finger_cnt > 0) {
      // Expect no timeout
      EXPECT_LT(timeout, 0);
    } else {
      // No timeout b/c it's right click. Expect the right click gesture
      ASSERT_NE(static_cast<Gesture*>(NULL), gs) << "timeout:" << timeout;
      EXPECT_EQ(kGestureTypeButtonsChange, gs->type);
      EXPECT_EQ(GESTURES_BUTTON_RIGHT, gs->details.buttons.down);
      EXPECT_EQ(GESTURES_BUTTON_RIGHT, gs->details.buttons.up);
      reset_next_time = true;  // All done w/ this run.
    }
  }
}

// Does two tap gestures, one with keyboard interference.
TEST(ImmediateInterpreterTest, TapToClickKeyboardTest) {
  std::unique_ptr<ImmediateInterpreter> ii;

  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    200,  // right edge
    200,  // bottom edge
    1.0,  // pixels/TP width
    1.0,  // pixels/TP height
    1.0,  // screen DPI x
    1.0,  // screen DPI y
    -1,  // orientation minimum
    2,   // orientation maximum
    5,  // max fingers
    5,  // max touch
    0,  // t5r2
    0,  // semi-mt
    1,  // is button pad
    0   // has_wheel
  };
  TestInterpreterWrapper wrapper(ii.get(), &hwprops);

  FingerState fs = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID
    0, 0, 0, 0, 50, 0, 4, 4, 91, 0
  };
  HardwareState hwstates[] = {
    // Simple 1-finger tap
    { 0.01, 0, 1, 1, &fs, 0, 0, 0, 0 },
    { 0.02, 0, 0, 0, NULL, 0, 0, 0, 0 },
    { 0.30, 0, 0, 0, NULL, 0, 0, 0, 0 }
  };

  enum {
    kWithoutKeyboard = 0,
    kWithKeyboard,
    kMaxTests
  };
  for (size_t test = 0; test != kMaxTests; test++) {
    ii.reset(new ImmediateInterpreter(NULL, NULL));
    wrapper.Reset(ii.get());
    ii->motion_tap_prevent_timeout_.val_ = 0;
    ii->tap_enable_.val_ = 1;
    ii->tap_drag_enable_.val_ = 1;

    if (test == kWithKeyboard)
      ii->keyboard_touched_ = 0.001;

    unsigned down = 0;
    unsigned up = 0;
    for (size_t i = 0; i < arraysize(hwstates); i++) {
      down = 0;
      up = 0;
      stime_t timeout = -1.0;
      set<short, kMaxGesturingFingers> gs =
          hwstates[i].finger_cnt == 1 ? MkSet(91) : MkSet();
      ii->UpdateTapState(
          &hwstates[i],
          gs,
          false,  // same fingers
          hwstates[i].timestamp,
          &down,
          &up,
          &timeout);
    }
    EXPECT_EQ(down, up);
    if (test == kWithoutKeyboard)
      EXPECT_EQ(GESTURES_BUTTON_LEFT, down);
    else
      EXPECT_EQ(0, down);
  }
}

TEST(ImmediateInterpreterTest, TapToClickEnableTest) {
  std::unique_ptr<ImmediateInterpreter> ii;

  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    200,  // right edge
    200,  // bottom edge
    1.0,  // pixels/TP width
    1.0,  // pixels/TP height
    1.0,  // screen DPI x
    1.0,  // screen DPI y
    -1,  // orientation minimum
    2,   // orientation maximum
    5,  // max fingers
    5,  // max touch
    0,  // t5r2
    0,  // semi-mt
    1,  // is button pad
    0   // has_wheel
  };
  TestInterpreterWrapper wrapper(ii.get(), &hwprops);

  FingerState fs[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID
    {0, 0, 0, 0, 50, 0, 4, 4, 91, 0},
    {0, 0, 0, 0, 50, 0, 4, 4, 92, 0},
    {0, 0, 0, 0, 50, 0, 6, 4, 92, 0},
    {0, 0, 0, 0, 50, 0, 8, 4, 92, 0},
    {0, 0, 0, 0, 50, 0, 4, 4, 93, 0},
    {0, 0, 0, 0, 50, 0, 6, 4, 93, 0},
    {0, 0, 0, 0, 50, 0, 8, 4, 93, 0},
  };

  HWStateGs hwsgs_list[] = {
    // 1-finger tap, move, release, move again (drag lock)
    {S,{0.00,0,1,1,&fs[0],0,0,0,0},-1,MkSet(91),0,0,kFTB,false},
    {C,{0.01,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kTpC,true},
    {C,{0.02,0,1,1,&fs[1],0,0,0,0},-1,MkSet(92),0,0,kSTB,false},
    {C,{0.08,0,1,1,&fs[2],0,0,0,0},-1,MkSet(92),kBL,0,kDrg,false},
    {C,{0.09,0,1,1,&fs[3],0,0,0,0},-1,MkSet(92),0,0,kDrg,false},
    {C,{0.10,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kDRl,true},
    {C,{0.11,0,1,1,&fs[4],0,0,0,0},-1,MkSet(93),0,0,kDRt,false},
    {C,{0.12,0,1,1,&fs[5],0,0,0,0},-1,MkSet(93),0,0,kDrg,false},
    {C,{0.13,0,1,1,&fs[6],0,0,0,0},-1,MkSet(93),0,0,kDrg,false},
    {C,{0.14,0,0,0,NULL,0,0,0,0},-1,MkSet(),0,0,kDRl,true},
    {C,{0.99,0,0,0,NULL,0,0,0,0},.99,MkSet(),0,kBL,kIdl,false}
  };

  for (int iter = 0; iter < 5; ++iter) {
    for (size_t i = 0; i < arraysize(hwsgs_list); ++i) {
      string desc;
      stime_t disable_time = 0.0;
      stime_t pause_time = 0.0;
      switch (iter) {
        case 0:  // test with tap enabled
          desc = StringPrintf("State %zu (tap enabled)", i);
          disable_time = -1;  // unreachable time
          pause_time = -1;
          break;
        case 1:  // test with tap disabled during gesture
          desc = StringPrintf("State %zu (tap disabled during gesture)", i);
          disable_time = 0.02;
          pause_time = -1;
          break;
        case 2:  // test with tap disabled before gesture (while Idle)
          desc = StringPrintf("State %zu (tap disabled while Idle)", i);
          disable_time = 0.00;
          pause_time = -1;
          break;
        case 3:  // test with tap paused during gesture
          desc = StringPrintf("State %zu (tap paused during gesture)", i);
          disable_time = -1;
          pause_time = 0.02;
          break;
        case 4:  // test with tap paused before gesture (while Idle)
          desc = StringPrintf("State %zu (tap paused while Idle)", i);
          disable_time = 0.00;
          pause_time = -1;
          break;
      }

      HWStateGs &hwsgs = hwsgs_list[i];
      HardwareState* hwstate = &hwsgs.hws;
      stime_t now = hwsgs.callback_now;
      if (hwsgs.callback_now >= 0.0)
        hwstate = NULL;
      else
        now = hwsgs.hws.timestamp;

      bool same_fingers = false;
      if (hwstate && hwstate->timestamp == 0.0) {
        // Reset imm interpreter
        fprintf(stderr, "Resetting imm interpreter, i = %zd\n", i);
        ii.reset(new ImmediateInterpreter(NULL, NULL));
        wrapper.Reset(ii.get());
        ii->drag_lock_enable_.val_ = 1;
        ii->motion_tap_prevent_timeout_.val_ = 0;
        ii->tap_drag_timeout_.val_ = 0.05;
        ii->tap_enable_.val_ = 1;
        ii->tap_drag_enable_.val_ = 1;
        ii->tap_paused_.val_ = 0;
        ii->tap_move_dist_.val_ = 1.0;
        ii->tap_timeout_.val_ = 0.05;
        EXPECT_EQ(kIdl, ii->tap_to_click_state_);
        EXPECT_TRUE(ii->tap_enable_.val_);
      } else {
        same_fingers = ii->state_buffer_.Get(1)->SameFingersAs(hwsgs.hws);
      }

      // Disable tap in the middle of the gesture
      if (hwstate && hwstate->timestamp == disable_time)
        ii->tap_enable_.val_ = 0;

      if (hwstate && hwstate->timestamp == pause_time)
        ii->tap_paused_.val_ = true;

      if (hwstate)
        ii->state_buffer_.PushState(*hwstate);
      unsigned bdown = 0;
      unsigned bup = 0;
      stime_t tm = -1.0;
      ii->UpdateTapState(
          hwstate, hwsgs.gs, same_fingers, now, &bdown, &bup, &tm);
      ii->prev_gs_fingers_ = hwsgs.gs;

      switch (iter) {
        case 0:  // tap should be enabled
        case 1:
        case 3:
          EXPECT_EQ(hwsgs.expected_down, bdown) << desc;
          EXPECT_EQ(hwsgs.expected_up, bup) << desc;
          if (hwsgs.timeout)
            EXPECT_GT(tm, 0.0) << desc;
          else
            EXPECT_DOUBLE_EQ(-1.0, tm) << desc;
          EXPECT_EQ(hwsgs.expected_state, ii->tap_to_click_state_) << desc;
          break;
        case 2:  // tap should be disabled
        case 4:
          EXPECT_EQ(0, bdown) << desc;
          EXPECT_EQ(0, bup) << desc;
          EXPECT_DOUBLE_EQ(-1.0, tm) << desc;
          EXPECT_EQ(kIdl, ii->tap_to_click_state_) << desc;
          break;
      }
    }
  }
}

struct ClickTestHardwareStateAndExpectations {
  HardwareState hs;
  stime_t timeout;
  unsigned expected_down;
  unsigned expected_up;
};

TEST(ImmediateInterpreterTest, ClickTest) {
  ImmediateInterpreter ii(NULL, NULL);
  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    100,  // right edge
    100,  // bottom edge
    1,  // x pixels/mm
    1,  // y pixels/mm
    96,  // x screen DPI
    96,  // y screen DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    2,  // max fingers
    5,  // max touch
    0,  // t5r2
    0,  // semi-mt
    1,  // is button pad
    0   // has_wheel
  };
  TestInterpreterWrapper wrapper(&ii, &hwprops);
  EXPECT_FLOAT_EQ(10.0, ii.tapping_finger_min_separation_.val_);

  FingerState finger_states[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID
    {0, 0, 0, 0, 10, 0, 50, 50, 1, 0},
    {0, 0, 0, 0, 10, 0, 70, 50, 2, 0},
    // Fingers very close together - shouldn't right click
    {0, 0, 0, 0, 10, 0, 50, 50, 1, 0},
    {0, 0, 0, 0, 10, 0, 55, 50, 2, 0},
    // Large vertical dist - shouldn right click when timing is good.
    {0, 0, 0, 0, 10, 0,  8.4, 94, 1, 0},
    {0, 0, 0, 0, 10, 0, 51.2, 70, 2, 0},
  };
  ClickTestHardwareStateAndExpectations records[] = {
    // reset
    {{0,0,0,0,NULL,0,0,0,0},-1,0,0},

    // button down, 2 fingers touch, button up, 2 fingers lift
    {{1,1,0,0,NULL,0,0,0,0},-1,0,0},
    {{1.01,1,2,2,&finger_states[0],0,0,0,0},-1,0,0},
    {{2,0,2,2,&finger_states[0],0,0,0,0},
     -1,GESTURES_BUTTON_RIGHT,GESTURES_BUTTON_RIGHT},
    {{3,0,0,0,NULL,0,0,0,0},-1,0,0},

    // button down, 2 close fingers touch, fingers lift
    {{7,1,0,0,NULL,0,0,0,0},-1,0,0},
    {{7.01,1,2,2,&finger_states[2],0,0,0,0},-1,0,0},
    {{7.02,0,2,2,&finger_states[2],0,0,0,0},
     -1,GESTURES_BUTTON_LEFT,GESTURES_BUTTON_LEFT},
    {{8,0,0,0,NULL,0,0,0,0},-1,0,0},

    // button down with 2 fingers, button up, fingers lift
    {{9.01,1,2,2,&finger_states[4],0,0,0,0},-1,0,0},
    {{9.02,1,2,2,&finger_states[4],0,0,0,0},-1,0,0},
    {{9.5,0,2,2,&finger_states[4],0,0,0,0},
     -1,GESTURES_BUTTON_RIGHT,GESTURES_BUTTON_RIGHT},
    {{10,0,0,0,NULL,0,0,0,0},-1,0,0},

    // button down with 2 fingers, timeout, button up, fingers lift
    {{11,1,2,2,&finger_states[4],0,0,0,0},-1,0,0},
    {{0,0,0,0,NULL,0,0,0,0},11.5,GESTURES_BUTTON_RIGHT,0},
    {{12,0,2,2,&finger_states[4],0,0,0,0},
      -1,0,GESTURES_BUTTON_RIGHT},
    {{10,0,0,0,NULL,0,0,0,0},-1,0,0}
  };

  for (size_t i = 0; i < arraysize(records); ++i) {
    Gesture* result = NULL;
    if (records[i].timeout < 0.0)
      result = wrapper.SyncInterpret(&records[i].hs, NULL);
    else
      result = wrapper.HandleTimer(records[i].timeout, NULL);
    if (records[i].expected_down == 0 && records[i].expected_up == 0) {
      EXPECT_EQ(static_cast<Gesture*>(NULL), result) << "i=" << i;
    } else {
      ASSERT_NE(static_cast<Gesture*>(NULL), result) << "i=" << i;
      EXPECT_EQ(records[i].expected_down, result->details.buttons.down);
      EXPECT_EQ(records[i].expected_up, result->details.buttons.up);
    }
  }
}

struct BigHandsRightClickInputAndExpectations {
  HardwareState hs;
  unsigned out_buttons_down;
  unsigned out_buttons_up;
  FingerState fs[2];
};

// This was recorded from Ian Fette, who had trouble right-clicking.
// This test plays back part of his log to ensure that it generates a
// right click.
TEST(ImmediateInterpreterTest, BigHandsRightClickTest) {
  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    106.666672,  // right edge
    68.000000,  // bottom edge
    1,  // pixels/TP width
    1,  // pixels/TP height
    25.4,  // screen DPI x
    25.4,  // screen DPI y
    -1,  // orientation minimum
    2,   // orientation maximum
    15,  // max fingers
    5,  // max touch
    0,  // t5r2
    0,  // semi-mt
    true,  // is button pad
    0   // has_wheel
  };
  BigHandsRightClickInputAndExpectations records[] = {
    { { 1329527921.327647, 0, 2, 2, NULL, 0, 0, 0, 0 }, 0, 0,
      { { 0, 0, 0, 0, 50.013428, 0, 20.250002, 59.400002, 130, 0 },
        { 0, 0, 0, 0, 41.862095, 0, 57.458694, 43.700001, 131, 0 } } },
    { { 1329527921.344421, 0, 2, 2, NULL, 0, 0, 0, 0 }, 0, 0,
      { { 0, 0, 0, 0, 50.301102, 0, 20.250002, 59.400002, 130, 0 },
        { 0, 0, 0, 0, 42.007469, 0, 57.479977, 43.700001, 131, 0 } } },
    { { 1329527921.361196,1,2,2,NULL,0,0,0,0 }, 0, 0,
      { { 0, 0, 0, 0, 50.608433, 0, 20.250002, 59.400002, 130, 0 },
        { 0, 0, 0, 0, 42.065464, 0, 57.494164, 43.700001, 131, 0 } } },
    { { 1329527921.372364, 1, 2, 2, NULL, 0, 0, 0, 0 }, 0, 0,
      { { 0, 0, 0, 0, 50.840954, 0, 20.250002, 59.400002, 130, 0 },
        { 0, 0, 0, 0, 42.071739, 0, 57.507217, 43.700001, 131, 0 } } },
    { { 1329527921.383517, 1, 2, 2, NULL, 0, 0, 0, 0 }, 0, 0,
      { { 0, 0, 0, 0, 51.047310, 0, 20.250002, 59.400002, 130, 0 },
        { 0, 0, 0, 0, 42.054974, 0, 57.527523, 43.700001, 131, 0 } } },
    { { 1329527921.394680, 1, 2, 2, NULL, 0, 0, 0, 0 }, 0, 0,
      { { 0, 0, 0, 0, 51.355824, 0, 20.250002, 59.400002, 130, 0 },
        { 0, 0, 0, 0, 42.066948, 0, 57.550964, 43.700001, 131, 0 } } },
    { { 1329527921.405842, 1, 2, 2, NULL, 0, 0, 0, 0 }, 0, 0,
      { { 0, 0, 0, 0, 51.791901, 0, 20.250002, 59.400002, 130, 0 },
        { 0, 0, 0, 0, 42.188736, 0, 57.569374, 43.700001, 131, 0 } } },
    { { 1329527921.416791, 1, 2, 2, NULL, 0, 0, 0, 0 },GESTURES_BUTTON_RIGHT,0,
      { { 0, 0, 0, 0, 52.264156, 0, 20.250002, 59.400002, 130, 0 },
        { 0, 0, 0, 0, 42.424179, 0, 57.586361, 43.700001, 131, 0 } } },
    { { 1329527921.427937, 1, 2, 2, NULL, 0, 0, 0, 0 }, 0, 0,
      { { 0, 0, 0, 0, 52.725105, 0, 20.250002, 59.400002, 130, 0 },
        { 0, 0, 0, 0, 42.676739, 0, 57.609421, 43.700001, 131, 0 } } },
    { { 1329527921.439094, 1, 2, 2, NULL, 0, 0, 0, 0 }, 0, 0,
      { { 0, 0, 0, 0, 53.191925, 0, 20.250002, 59.400002, 130, 0 },
        { 0, 0, 0, 0, 42.868217, 0, 57.640007, 43.700001, 131, 0 } } },
    { { 1329527921.461392, 1, 2, 2, NULL, 0, 0, 0, 0 }, 0, 0,
      { { 0, 0, 0, 0, 53.602665, 0, 20.250002, 59.400002, 130, 0 },
        { 0, 0, 0, 0, 43.016544, 0, 57.676689, 43.700001, 131, 0 } } },
    { { 1329527921.483690, 1, 2, 2, NULL, 0, 0, 0, 0 }, 0, 0,
      { { 0, 0, 0, 0, 53.879429, 0, 20.250002, 59.400002, 130, 0 },
        { 0, 0, 0, 0, 43.208221, 0, 57.711613, 43.700001, 131, 0 } } },
    { { 1329527921.511815, 1, 2, 2, NULL, 0, 0, 0, 0 }, 0, 0,
      { { 0, 0, 0, 0, 54.059937, 0, 20.250002, 59.400002, 130, 0 },
        { 0, 0, 0, 0, 43.467258, 0, 57.736385, 43.700001, 131, 0 } } },
    { { 1329527921.539940, 1, 2, 2, NULL, 0, 0, 0, 0 }, 0, 0,
      { { 0, 0, 0, 0, 54.253189, 0, 20.250002, 59.400002, 130, 0 },
        { 0, 0, 0, 0, 43.717934, 0, 57.750286, 43.700001, 131, 0 } } },
    { { 1329527921.556732, 1, 2, 2, NULL, 0, 0, 0, 0 }, 0, 0,
      { { 0, 0, 0, 0, 54.500740, 0, 20.250002, 59.400002, 130, 0 },
        { 0, 0, 0, 0, 43.863792, 0, 57.758759, 43.700001, 131, 0 } } },
    { { 1329527921.573523, 1, 2, 2, NULL, 0, 0, 0, 0 }, 0, 0,
      { { 0, 0, 0, 0, 54.737640, 0, 20.250002, 59.400002, 130, 0 },
        { 0, 0, 0, 0, 43.825844, 0, 57.771137, 43.700001, 131, 0 } } },
    { { 1329527921.584697, 1, 2, 2, NULL, 0, 0, 0, 0 }, 0, 0,
      { { 0, 0, 0, 0, 54.906223, 0, 20.250002, 59.400002, 130, 0 },
        { 0, 0, 0, 0, 43.654804, 0, 57.790218, 43.700001, 131, 0 } } },
    { { 1329527921.595872, 1, 2, 2, NULL, 0, 0, 0, 0 }, 0, 0,
      { { 0, 0, 0, 0, 55.001118, 0, 20.250002, 59.400002, 130, 0 },
        { 0, 0, 0, 0, 43.542431, 0, 57.809731, 43.700001, 131, 0 } } },
    { { 1329527921.618320, 1, 2, 2, NULL, 0, 0, 0, 0 }, 0, 0,
      { { 0, 0, 0, 0, 55.039989, 0, 20.252811, 59.400002, 130, 0 },
        { 0, 0, 0, 0, 43.585777, 0, 57.824154, 43.700001, 131, 0 } } },
    { { 1329527921.640768, 1, 2, 2, NULL, 0, 0, 0, 0 }, 0, 0,
      { { 0, 0, 0, 0, 55.045246, 0, 20.264456, 59.400002, 130, 0 },
        { 0, 0, 0, 0, 43.715435, 0, 57.832584, 43.700001, 131, 0 } } },
    { { 1329527921.691161, 1, 2, 2, NULL, 0, 0, 0, 0 }, 0, 0,
      { { 0, 0, 0, 0, 55.068935, 0, 20.285036, 59.400002, 130, 0 },
        { 0, 0, 0, 0, 43.845741, 0, 57.836266, 43.700001, 131, 0 } } },
    { { 1329527921.741554, 1, 2, 2, NULL, 0, 0, 0, 0 }, 0, 0,
      { { 0, 0, 0, 0, 55.195026, 0, 20.306564, 59.400002, 130, 0 },
        { 0, 0, 0, 0, 43.941154, 0, 57.836994, 43.700001, 131, 0 } } },
    { { 1329527921.758389, 1, 2, 2, NULL, 0, 0, 0, 0 }, 0, 0,
      { { 0, 0, 0, 0, 55.430550, 0, 20.322674, 59.400002, 130, 0 },
        { 0, 0, 0, 0, 43.962692, 0, 57.836308, 43.700001, 131, 0 } } },
    { { 1329527921.775225, 1, 2, 2, NULL, 0, 0, 0, 0 }, 0, 0,
      { { 0, 0, 0, 0, 55.681423, 0, 20.332201, 59.400002, 130, 0 },
        { 0, 0, 0, 0, 43.846741, 0, 57.835224, 43.700001, 131, 0 } } },
    { { 1329527921.786418, 1, 2, 2, NULL, 0, 0, 0, 0 }, 0, 0,
      { { 0, 0, 0, 0, 55.803486, 0, 20.336439, 59.400002, 130, 0 },
        { 0, 0, 0, 0, 43.604134, 0, 57.834267, 43.700001, 131, 0 } } },
    { { 1329527921.803205, 1, 2, 2, NULL, 0, 0, 0, 0 }, 0, 0,
      { { 0, 0, 0, 0, 55.738258, 0, 20.337351, 59.396629, 130, 0 },
        { 0, 0, 0, 0, 43.340977, 0, 57.833622, 43.700001, 131, 0 } } },
    { { 1329527921.819993, 1, 2, 2, NULL, 0, 0, 0, 0 }, 0, 0,
      { { 0, 0, 0, 0, 55.647045, 0, 20.336643, 59.382656, 130, 0 },
        { 0, 0, 0, 0, 43.140343, 0, 57.833279, 43.700001, 131, 0 } } },
    { { 1329527921.831121, 1, 2, 2, NULL, 0, 0, 0, 0 }, 0, 0,
      { { 0, 0, 0, 0, 55.670898, 0, 20.335459, 59.357960, 130, 0 },
        { 0, 0, 0, 0, 43.019653, 0, 57.827530, 43.700001, 131, 0 } } },
    { { 1329527921.842232, 1, 2, 2, NULL, 0, 0, 0, 0 }, 0, 0,
      { { 0, 0, 0, 0, 55.769543, 0, 20.334396, 59.332127, 130, 0 },
        { 0, 0, 0, 0, 42.964531, 0, 57.807049, 43.700001, 131, 0 } } },
    { { 1329527921.853342, 1, 2, 2, NULL, 0, 0, 0, 0 }, 0, 0,
      { { 0, 0, 0, 0, 55.872444, 0, 20.333672, 59.312794, 130, 0 },
        { 0, 0, 0, 0, 42.951347, 0, 57.771957, 43.700001, 131, 0 } } },
    { { 1329527921.864522, 1, 2, 2, NULL, 0, 0, 0, 0 }, 0, 0,
      { { 0, 0, 0, 0, 55.949341, 0, 20.333281, 59.301361, 130, 0 },
        { 0, 0, 0, 0, 42.959034, 0, 57.729061, 43.700001, 131, 0 } } },
    { { 1329527921.875702, 1, 2, 2, NULL, 0, 0, 0, 0 }, 0, 0,
      { { 0, 0, 0, 0, 55.994751, 0, 20.333134, 59.296276, 130, 0 },
        { 0, 0, 0, 0, 42.973259, 0, 57.683277, 43.700001, 131, 0 } } },
    { { 1329527921.886840, 1, 2, 2, NULL, 0, 0, 0, 0 }, 0, 0,
      { { 0, 0, 0, 0, 56.014912, 0, 20.333128, 59.295181, 130, 0 },
        { 0, 0, 0, 0, 42.918892, 0, 57.640221, 43.700001, 131, 0 } } },
    { { 1329527921.898031, 1, 2, 2, NULL, 0, 0, 0, 0 }, 0, 0,
      { { 0, 0, 0, 0, 55.951756, 0, 20.333181, 59.296028, 130, 0 },
        { 0, 0, 0, 0, 42.715969, 0, 57.601479, 43.700001, 131, 0 } } },
    { { 1329527921.909149, 1, 2, 2, NULL, 0, 0, 0, 0 }, 0, 0,
      { { 0, 0, 0, 0, 55.736336, 0, 20.333244, 59.297451, 130, 0 },
        { 0, 0, 0, 0, 42.304108, 0, 57.563725, 43.700001, 131, 0 } } },
    { { 1329527921.920301,0,2,2,NULL,0,0,0,0 },0,GESTURES_BUTTON_RIGHT,
      { { 0, 0, 0, 0, 55.448730, 0, 20.333294, 59.298725, 130, 0 },
        { 0, 0, 0, 0, 41.444939, 0, 57.525326, 43.700001, 131, 0 } } }
  };
  ImmediateInterpreter ii(NULL, NULL);
  TestInterpreterWrapper wrapper(&ii, &hwprops);
  for (size_t i = 0; i < arraysize(records); i++) {
    // Make the hwstate point to the fingers
    HardwareState* hs = &records[i].hs;
    hs->fingers = records[i].fs;
    Gesture* gs_out = wrapper.SyncInterpret(hs, NULL);
    if (!gs_out || gs_out->type != kGestureTypeButtonsChange) {
      // We got no output buttons gesture. Make sure we expected that
      EXPECT_EQ(0, records[i].out_buttons_down);
      EXPECT_EQ(0, records[i].out_buttons_up);
    } else if (gs_out) {
      // We got a buttons gesture
      EXPECT_EQ(gs_out->details.buttons.down, records[i].out_buttons_down);
      EXPECT_EQ(gs_out->details.buttons.up, records[i].out_buttons_up);
    } else {
      ADD_FAILURE();  // This should be unreachable
    }
  }
}

TEST(ImmediateInterpreterTest, ChangeTimeoutTest) {
  ImmediateInterpreter ii(NULL, NULL);
  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    1000,  // right edge
    1000,  // bottom edge
    500,  // pixels/TP width
    500,  // pixels/TP height
    96,  // screen DPI x
    96,  // screen DPI y
    -1,  // orientation minimum
    2,   // orientation maximum
    2,  // max fingers
    5,  // max touch
    0,  // tripletap
    0,  // semi-mt
    1,  // is button pad
    0   // has_wheel
  };

  FingerState finger_states[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID
    {0, 0, 0, 0, 21, 0, 30, 30, 2, 0},
    {0, 0, 0, 0, 21, 0, 10, 10, 1, 0},
    {0, 0, 0, 0, 21, 0, 10, 20, 1, 0},
    {0, 0, 0, 0, 21, 0, 20, 20, 1, 0}
  };
  HardwareState hardware_states[] = {
    // time, buttons down, finger count, finger states pointer
    // One finger moves
    { 0.10, 0, 1, 1, &finger_states[1], 0, 0, 0, 0 },
    { 0.12, 0, 1, 1, &finger_states[2], 0, 0, 0, 0 },
    { 0.16, 0, 1, 1, &finger_states[3], 0, 0, 0, 0 },
    { 0.5,  0, 0, 0, NULL, 0, 0, 0, 0 },
    // One finger moves after another finger leaves
    { 1.09, 0, 2, 2, &finger_states[0], 0, 0, 0, 0 },
    { 1.10, 0, 1, 1, &finger_states[1], 0, 0, 0, 0 },
    { 1.12, 0, 1, 1, &finger_states[2], 0, 0, 0, 0 },
    { 1.16, 0, 1, 1, &finger_states[3], 0, 0, 0, 0 },
    { 1.5,  0, 0, 0, NULL, 0, 0, 0, 0 },
  };

  TestInterpreterWrapper wrapper(&ii, &hwprops);

  EXPECT_EQ(NULL, wrapper.SyncInterpret(&hardware_states[0], NULL));

  // One finger moves, change_timeout_ is not applied.
  Gesture* gs = wrapper.SyncInterpret(&hardware_states[1], NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), gs);
  EXPECT_EQ(kGestureTypeMove, gs->type);
  EXPECT_EQ(0, gs->details.move.dx);
  EXPECT_EQ(10, gs->details.move.dy);
  EXPECT_EQ(0.10, gs->start_time);
  EXPECT_EQ(0.12, gs->end_time);

  // One finger moves, change_timeout_ does not block the gesture.
  gs = wrapper.SyncInterpret(&hardware_states[2], NULL);
  EXPECT_NE(reinterpret_cast<Gesture*>(NULL), gs);
  EXPECT_EQ(kGestureTypeMove, gs->type);
  EXPECT_EQ(10, gs->details.move.dx);
  EXPECT_EQ(0, gs->details.move.dy);
  EXPECT_EQ(0.12, gs->start_time);
  EXPECT_EQ(0.16, gs->end_time);

  // No finger.
  EXPECT_EQ(NULL, wrapper.SyncInterpret(&hardware_states[3], NULL));

  // Start with 2 fingers.
  EXPECT_EQ(NULL, wrapper.SyncInterpret(&hardware_states[4], NULL));
  // One finger leaves, finger_leave_time_ recorded.
  EXPECT_EQ(NULL, wrapper.SyncInterpret(&hardware_states[5], NULL));
  EXPECT_EQ(ii.finger_leave_time_, 1.10);
  // One finger moves, change_timeout_ blocks the gesture.
  EXPECT_EQ(NULL, wrapper.SyncInterpret(&hardware_states[6], NULL));

  // One finger moves, finger_leave_time_ + change_timeout_
  // has been passed.
  gs = wrapper.SyncInterpret(&hardware_states[7], NULL);
  EXPECT_NE(reinterpret_cast<Gesture*>(NULL), gs);
  EXPECT_EQ(kGestureTypeMove, gs->type);
  EXPECT_EQ(10, gs->details.move.dx);
  EXPECT_EQ(0, gs->details.move.dy);
  EXPECT_EQ(1.12, gs->start_time);
  EXPECT_EQ(1.16, gs->end_time);

  EXPECT_EQ(NULL, wrapper.SyncInterpret(&hardware_states[8], NULL));
}

// Tests that fingers that have been present a while, but are stationary,
// can be evaluated multiple times when they start moving.

enum PinchTestExpectedResult {
  kPinch,
  kNoPinch,
  kAny
};

struct PinchTestInput {
  HardwareState hs;
  PinchTestExpectedResult expected_result;
};

TEST(ImmediateInterpreterTest, DISABLED_PinchTests) {
  ImmediateInterpreter ii(NULL, NULL);
  ii.pinch_enable_.val_ = 1;
  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    100,  // right edge
    100,  // bottom edge
    1,  // pixels/TP width
    1,  // pixels/TP height
    96,  // x screen DPI
    96,  // y screen DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    2,  // max fingers
    5,  // max touch
    0,  // tripletap
    0,  // semi-mt
    1,  // is button pad
    0   // has_wheel
  };

  FingerState finger_states[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID
    // 0: start position
    {0, 0, 0, 0, 20, 0, 40, 40, 1, 0},
    {0, 0, 0, 0, 20, 0, 90, 90, 2, 0},

    // 2: pinch inwards
    {0, 0, 0, 0, 20, 0, 41, 41, 1, 0},
    {0, 0, 0, 0, 20, 0, 89, 89, 2, 0},
    {0, 0, 0, 0, 20, 0, 42, 42, 1, 0},
    {0, 0, 0, 0, 20, 0, 88, 88, 2, 0},
    {0, 0, 0, 0, 20, 0, 43, 43, 1, 0},
    {0, 0, 0, 0, 20, 0, 87, 87, 2, 0},
    {0, 0, 0, 0, 20, 0, 44, 44, 1, 0},
    {0, 0, 0, 0, 20, 0, 86, 86, 2, 0},
    {0, 0, 0, 0, 20, 0, 45, 45, 1, 0},
    {0, 0, 0, 0, 20, 0, 85, 85, 2, 0},

    // 12: pinch outwards
    {0, 0, 0, 0, 20, 0, 39, 39, 1, 0},
    {0, 0, 0, 0, 20, 0, 91, 91, 2, 0},
    {0, 0, 0, 0, 20, 0, 38, 38, 1, 0},
    {0, 0, 0, 0, 20, 0, 92, 92, 2, 0},
    {0, 0, 0, 0, 20, 0, 37, 37, 1, 0},
    {0, 0, 0, 0, 20, 0, 93, 93, 2, 0},
    {0, 0, 0, 0, 20, 0, 36, 36, 1, 0},
    {0, 0, 0, 0, 20, 0, 94, 94, 2, 0},
    {0, 0, 0, 0, 20, 0, 35, 35, 1, 0},
    {0, 0, 0, 0, 20, 0, 95, 95, 2, 0},

    // 22: single finger pinch
    {0, 0, 0, 0, 20, 0, 39, 39, 1, 0},
    {0, 0, 0, 0, 20, 0, 90, 90, 2, 0},
    {0, 0, 0, 0, 20, 0, 38, 38, 1, 0},
    {0, 0, 0, 0, 20, 0, 90, 90, 2, 0},
    {0, 0, 0, 0, 20, 0, 37, 37, 1, 0},
    {0, 0, 0, 0, 20, 0, 90, 90, 2, 0},
    {0, 0, 0, 0, 20, 0, 36, 36, 1, 0},
    {0, 0, 0, 0, 20, 0, 90, 90, 2, 0},
    {0, 0, 0, 0, 20, 0, 35, 35, 1, 0},
    {0, 0, 0, 0, 20, 0, 90, 90, 2, 0},
  };

  PinchTestInput input_states[] = {
    // time, buttons, finger count, touch count, finger states pointer
    {{ 0.00, 0, 0, 0, NULL, 0, 0, 0, 0 }, kAny},

    // fast pinch outwards
    {{ 0.11, 0, 2, 2, &finger_states[0], 0, 0, 0, 0 }, kAny},
    {{ 0.12, 0, 2, 2, &finger_states[4], 0, 0, 0, 0 }, kAny},
    {{ 0.13, 0, 2, 2, &finger_states[8], 0, 0, 0, 0 }, kAny},
    {{ 0.14, 0, 2, 2, &finger_states[10], 0, 0, 0, 0 }, kPinch},
    {{ 0.15, 0, 0, 0, NULL, 0, 0, 0, 0 }, kAny},

    // slow pinch
    {{ 1.01, 0, 2, 2, &finger_states[0], 0, 0, 0, 0 }, kAny},
    {{ 1.02, 0, 2, 2, &finger_states[0], 0, 0, 0, 0 }, kAny},
    {{ 1.03, 0, 2, 2, &finger_states[2], 0, 0, 0, 0 }, kAny},
    {{ 1.04, 0, 2, 2, &finger_states[2], 0, 0, 0, 0 }, kAny},
    {{ 1.05, 0, 2, 2, &finger_states[4], 0, 0, 0, 0 }, kAny},
    {{ 1.06, 0, 2, 2, &finger_states[4], 0, 0, 0, 0 }, kAny},
    {{ 1.07, 0, 2, 2, &finger_states[6], 0, 0, 0, 0 }, kAny},
    {{ 1.08, 0, 2, 2, &finger_states[6], 0, 0, 0, 0 }, kAny},
    {{ 1.09, 0, 2, 2, &finger_states[8], 0, 0, 0, 0 }, kAny},
    {{ 1.10, 0, 2, 2, &finger_states[8], 0, 0, 0, 0 }, kAny},
    {{ 1.11, 0, 2, 2, &finger_states[10], 0, 0, 0, 0 }, kAny},
    {{ 1.12, 0, 2, 2, &finger_states[10], 0, 0, 0, 0 }, kPinch},
    {{ 1.13, 0, 0, 0, NULL, 0, 0, 0, 0 }, kAny},

    // single finger pinch
    {{ 2.01, 0, 2, 2, &finger_states[22], 0, 0, 0, 0 }, kAny},
    {{ 2.02, 0, 2, 2, &finger_states[26], 0, 0, 0, 0 }, kAny},
    {{ 2.03, 0, 2, 2, &finger_states[30], 0, 0, 0, 0 }, kAny},
    {{ 2.04, 0, 2, 2, &finger_states[32], 0, 0, 0, 0 }, kNoPinch},
    {{ 2.05, 0, 0, 0, NULL, 0, 0, 0, 0 }, kAny},


    // first single finger pinch, then second moves too.
    {{ 3.01, 0, 2, 2, &finger_states[22], 0, 0, 0, 0 }, kAny},
    {{ 3.02, 0, 2, 2, &finger_states[24], 0, 0, 0, 0 }, kAny},
    {{ 3.03, 0, 2, 2, &finger_states[6], 0, 0, 0, 0 }, kAny},
    {{ 3.04, 0, 2, 2, &finger_states[8], 0, 0, 0, 0 }, kAny},
    {{ 3.05, 0, 2, 2, &finger_states[10], 0, 0, 0, 0 }, kPinch},
    {{ 3.06, 0, 0, 0, NULL, 0, 0, 0, 0 }, kAny},

    // fast pinch inwards
    {{ 4.01, 0, 2, 2, &finger_states[10], 0, 0, 0, 0 }, kAny},
    {{ 4.02, 0, 2, 2, &finger_states[8], 0, 0, 0, 0 }, kAny},
    {{ 4.03, 0, 2, 2, &finger_states[4], 0, 0, 0, 0 }, kAny},
    {{ 4.04, 0, 2, 2, &finger_states[0], 0, 0, 0, 0 }, kPinch},
    {{ 4.05, 0, 0, 0, NULL, 0, 0, 0, 0 }, kAny},
  };

  TestInterpreterWrapper wrapper(&ii, &hwprops);

  for (size_t idx = 0; idx < arraysize(input_states); ++idx) {
    Gesture* gs = wrapper.SyncInterpret(&input_states[idx].hs, NULL);
    // assert pinch detected
    if (input_states[idx].expected_result == kPinch) {
      ASSERT_NE(reinterpret_cast<Gesture*>(NULL), gs);
      EXPECT_EQ(kGestureTypePinch, gs->type) << "idx=" << idx;
    }
    // assert pinch not detected
    if (input_states[idx].expected_result == kNoPinch) {
      if (gs != NULL) {
        EXPECT_NE(kGestureTypePinch, gs->type);
      }
    }
  }
}

struct AvoidAccidentalPinchTestInput {
  TestCaseStartOrContinueFlag flag;
  stime_t now;
  float x0, y0, p0, x1, y1, p1;  // (x, y) coordinate + pressure per finger
  GestureType expected_gesture;
};

// These data are from real logs where a move with resting thumb was appempted,
// but pinch code prevented it.
TEST(ImmediateInterpreterTest, AvoidAccidentalPinchTest) {
  std::unique_ptr<ImmediateInterpreter> ii;
  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    106.666672,  // right edge
    68.000000,  // bottom edge
    1,  // pixels/TP width
    1,  // pixels/TP height
    25.4,  // screen DPI x
    25.4,  // screen DPI y
    -1,  // orientation minimum
    2,   // orientation maximum
    15,  // max fingers
    5,  // max touch
    0,  // t5r2
    0,  // semi-mt
    true,  // is button pad
    0   // has_wheel
  };
  TestInterpreterWrapper wrapper(ii.get(), &hwprops);

  const GestureType kMov = kGestureTypeMove;
  const GestureType kAny = kGestureTypeNull;

  AvoidAccidentalPinchTestInput inputs[] = {
    { kS, 0.97697, 44.08, 64.30, 118.20, 35.91, 27.70, 44.46, kAny },
    { kC, 0.98755, 44.08, 64.30, 118.20, 35.91, 27.70, 50.28, kAny },
    { kC, 0.99816, 44.08, 64.30, 118.20, 35.91, 27.70, 54.16, kAny },
    { kC, 1.00876, 45.33, 65.50,  98.79, 35.91, 27.60, 56.10, kAny },
    { kC, 1.01936, 45.33, 65.50,  98.79, 35.91, 27.60, 58.04, kAny },
    { kC, 1.03026, 45.33, 65.50, 100.73, 34.50, 26.70, 63.87, kAny },
    { kC, 1.04124, 45.33, 65.50, 102.67, 33.00, 26.10, 65.81, kMov },
    { kC, 1.05198, 45.33, 65.50, 102.67, 31.25, 25.60, 71.63, kMov },
    { kC, 1.06279, 45.33, 65.50, 104.61, 28.75, 25.10, 73.57, kMov },
    { kC, 1.07364, 45.33, 65.50, 104.61, 27.00, 24.60, 71.63, kMov },
    { kC, 1.08451, 45.33, 65.50, 104.61, 25.41, 24.10, 71.63, kMov },
    { kC, 1.09512, 45.33, 65.50, 102.67, 23.58, 23.50, 75.51, kMov },
    { kC, 1.10573, 45.33, 65.50, 104.61, 22.25, 23.30, 73.57, kMov },
    { kC, 1.11671, 45.33, 65.50, 104.61, 21.16, 23.20, 75.51, kMov },
    { kC, 1.12744, 45.33, 65.50, 104.61, 20.25, 23.20, 81.33, kMov },
    { kC, 1.13833, 45.33, 65.50, 104.61, 19.41, 23.20, 79.39, kMov },
    { kC, 1.14913, 45.33, 65.50, 104.61, 18.33, 23.20, 81.33, kMov },
    { kC, 1.15985, 45.41, 65.50, 104.61, 17.50, 23.40, 79.39, kMov },
    { kC, 1.17044, 45.58, 65.50, 106.55, 16.75, 23.80, 81.33, kMov },
    { kC, 1.18117, 45.58, 65.50, 106.55, 16.33, 24.20, 77.45, kMov },
    { kC, 1.19188, 45.58, 65.50, 106.55, 16.00, 24.30, 71.63, kMov },
    { kC, 1.20260, 45.58, 65.50, 106.55, 16.00, 24.50, 48.34, kAny },
    { kC, 1.21331, 45.58, 65.50, 106.55, 15.91, 24.80,  9.54, kAny },

    { kS, 3.92951, 58.50, 58.40, 118.20, 38.25, 14.40,  3.71, kAny },
    { kC, 3.94014, 58.33, 58.40, 118.20, 38.25, 14.40, 38.64, kAny },
    { kC, 3.95082, 58.25, 58.40, 118.20, 38.33, 14.40, 50.28, kAny },
    { kC, 3.96148, 58.08, 58.40, 118.20, 38.41, 14.40, 52.22, kAny },
    { kC, 3.97222, 57.91, 58.40, 118.20, 38.41, 14.50, 56.10, kAny },
    { kC, 3.98303, 57.83, 58.40, 118.20, 38.58, 14.60, 59.99, kAny },
    { kC, 3.99376, 57.66, 58.40, 118.20, 38.75, 14.80, 63.87, kAny },
    { kC, 4.00452, 57.58, 58.40, 118.20, 38.91, 15.00, 65.81, kAny },
    { kC, 4.01528, 57.50, 58.40, 118.20, 39.33, 15.30, 67.75, kAny },
    { kC, 4.02621, 57.41, 58.40, 118.20, 39.50, 15.70, 69.69, kAny },
    { kC, 4.03697, 57.41, 58.40, 118.20, 39.75, 16.10, 73.57, kMov },
    { kC, 4.04781, 57.25, 58.40, 118.20, 40.00, 16.60, 71.63, kMov },
    { kC, 4.05869, 57.25, 58.40, 118.20, 40.25, 17.00, 71.63, kMov },
    { kC, 4.06966, 57.25, 58.40, 118.20, 40.50, 17.30, 69.69, kMov },
    { kC, 4.08034, 57.16, 58.40, 118.20, 40.75, 17.70, 71.63, kMov },
    { kC, 4.09120, 57.08, 58.40, 118.20, 41.16, 17.90, 73.57, kMov },
    { kC, 4.10214, 57.00, 58.40, 118.20, 41.50, 18.30, 73.57, kMov },
    { kC, 4.11304, 56.83, 58.29, 118.20, 42.00, 18.60, 71.63, kMov },
    { kC, 4.12390, 56.83, 58.29, 118.20, 42.58, 19.00, 71.63, kMov },
    { kC, 4.13447, 56.66, 58.29, 118.20, 43.16, 19.60, 67.75, kMov },
    { kC, 4.14521, 56.66, 58.29, 118.20, 43.75, 20.20, 67.75, kMov },
    { kC, 4.15606, 56.50, 58.20, 118.20, 44.41, 21.10, 69.69, kMov },
    { kC, 4.16692, 56.50, 58.20, 118.20, 44.91, 22.10, 67.75, kMov },
    { kC, 4.17778, 56.41, 58.20, 118.20, 45.58, 23.00, 65.81, kMov },
    { kC, 4.18894, 56.33, 58.10, 118.20, 46.08, 23.60, 65.81, kMov },
    { kC, 4.20017, 56.33, 58.10, 118.20, 46.50, 24.10, 65.81, kMov },
    { kC, 4.21111, 56.33, 58.10, 118.20, 46.83, 24.50, 63.87, kMov },
    { kC, 4.22204, 56.33, 58.10, 118.20, 47.08, 24.80, 61.93, kMov },
    { kC, 4.23308, 56.25, 58.10, 118.20, 47.50, 25.20, 59.99, kMov },
    { kC, 4.24371, 56.25, 58.10, 118.20, 48.00, 25.80, 58.04, kMov },
    { kC, 4.25438, 56.25, 58.10, 118.20, 48.66, 26.50, 58.04, kMov },
    { kC, 4.26508, 56.08, 58.00, 118.20, 49.50, 27.50, 54.16, kMov },
    { kC, 4.27572, 56.00, 58.00, 118.20, 50.33, 28.60, 56.10, kMov },
    { kC, 4.28662, 56.00, 58.00, 118.20, 51.33, 29.50, 58.04, kMov },
    { kC, 4.29757, 55.91, 58.00, 118.20, 51.58, 31.90, 56.10, kMov },
    { kC, 4.30850, 55.91, 58.00, 118.20, 52.08, 32.00, 54.16, kMov },
    { kC, 4.31943, 55.91, 58.00, 118.20, 52.58, 32.40, 54.16, kMov },
    { kC, 4.33022, 55.83, 57.90, 118.20, 52.75, 33.10, 52.22, kMov },
    { kC, 4.34104, 55.83, 57.90, 118.20, 53.16, 33.60, 52.22, kMov },
    { kC, 4.35167, 55.83, 57.90, 118.20, 53.58, 34.20, 50.28, kMov },
    { kC, 4.36225, 55.83, 57.90, 118.20, 53.91, 35.00, 48.34, kMov },
    { kC, 4.37290, 55.75, 57.90, 118.20, 54.58, 35.50, 50.28, kMov },
    { kC, 4.38368, 55.66, 57.90, 118.20, 55.33, 36.10, 48.34, kMov },
    { kC, 4.39441, 55.66, 57.90, 118.20, 55.91, 36.70, 48.34, kMov },
    { kC, 4.40613, 56.16, 57.90, 118.20, 57.00, 37.20, 50.28, kMov },
    { kC, 4.41787, 56.16, 57.90, 118.20, 57.33, 37.70, 50.28, kMov },
    { kC, 4.42925, 56.16, 57.90, 118.20, 57.58, 37.90, 48.34, kMov },
    { kC, 4.44080, 56.16, 57.90, 118.20, 57.66, 38.00, 50.28, kMov },
    { kC, 4.45249, 56.16, 57.90, 118.20, 57.75, 38.10, 50.28, kMov },
    { kC, 4.46393, 56.16, 57.90, 118.20, 57.75, 38.10, 50.28, kAny },
    { kC, 4.47542, 56.16, 57.90, 118.20, 57.75, 38.15, 50.28, kMov },
    { kC, 4.48691, 56.16, 57.90, 118.20, 57.75, 38.20, 50.28, kMov },
    { kC, 4.49843, 56.16, 57.90, 118.20, 57.75, 38.20, 50.28, kAny },
    { kC, 4.51581, 56.16, 57.90, 118.20, 57.75, 38.25, 51.25, kMov },
    { kC, 4.53319, 56.16, 57.90, 118.20, 57.75, 38.29, 52.22, kMov },
    { kC, 4.54472, 56.16, 57.90, 118.20, 57.75, 38.70, 50.28, kMov },
    { kC, 4.55630, 56.16, 57.90, 118.20, 57.75, 38.70, 50.28, kAny },
    { kC, 4.56787, 56.16, 57.90, 118.20, 57.75, 38.70, 52.22, kAny },
    { kC, 4.57928, 56.16, 57.90, 118.20, 58.33, 38.50, 50.28, kMov },
    { kC, 4.59082, 56.16, 57.90, 118.20, 58.25, 38.60, 50.28, kMov },
    { kC, 4.60234, 56.16, 57.90, 118.20, 58.33, 38.60, 52.22, kMov },
    { kC, 4.61389, 56.16, 57.90, 118.20, 58.33, 38.60, 52.22, kAny },
    { kC, 4.62545, 56.16, 57.90, 118.20, 58.33, 38.60, 52.22, kAny },
    { kC, 4.64281, 56.16, 57.90, 118.20, 58.33, 38.60, 52.22, kAny },
    { kC, 4.66018, 56.16, 57.90, 118.20, 58.33, 38.60, 52.22, kAny },
    { kC, 4.67747, 56.16, 57.90, 118.20, 58.33, 38.60, 52.22, kAny },
    { kC, 4.69476, 56.16, 57.90, 118.20, 58.33, 38.60, 52.22, kAny },
    { kC, 4.70628, 56.16, 57.90, 118.20, 58.33, 38.60, 52.22, kAny },
    { kC, 4.71781, 56.16, 57.90, 118.20, 58.33, 38.60, 52.22, kAny },
    { kC, 4.72934, 56.16, 57.90, 118.20, 58.33, 38.60, 52.22, kAny },
    { kC, 4.74087, 56.16, 57.90, 118.20, 58.33, 38.60, 52.22, kAny },
    { kC, 4.75240, 56.16, 57.90, 118.20, 58.33, 38.60, 52.22, kAny },
    { kC, 4.76418, 56.16, 57.90, 118.20, 58.33, 38.60, 50.28, kAny },
    { kC, 4.77545, 56.08, 57.90, 118.20, 58.33, 38.60, 50.28, kAny },
    { kC, 4.78690, 56.08, 57.90, 118.20, 58.33, 38.60, 48.34, kAny },
    { kC, 4.79818, 56.08, 57.90, 118.20, 58.33, 38.60, 27.00, kAny },
    { kC, 4.80970, 56.08, 57.90, 118.20, 58.33, 38.60,  9.54, kAny },
  };

  for (size_t i = 0; i < arraysize(inputs); i++) {
    const AvoidAccidentalPinchTestInput& input = inputs[i];
    if (input.flag == kS) {
      ii.reset(new ImmediateInterpreter(NULL, NULL));
      ii->pinch_enable_.val_ = true;
      MetricsProperties* mprops = new MetricsProperties(NULL);
      mprops->two_finger_close_vertical_distance_thresh.val_ = 35.0;
      wrapper.Reset(ii.get(), mprops);
    }
    // Prep inputs
    FingerState fs[] = {
      { 0, 0, 0, 0, input.p0, 0, input.x0, input.y0, 1, 0 },
      { 0, 0, 0, 0, input.p1, 0, input.x1, input.y1, 2, 0 },
    };
    HardwareState hs = { input.now, 0, 2, 2, fs, 0, 0, 0, 0 };
    stime_t timeout = -1;
    Gesture* gs = wrapper.SyncInterpret(&hs, &timeout);
    if (input.expected_gesture != kAny) {
      if (gs)
        EXPECT_EQ(input.expected_gesture, gs->type);
    }
  }
}

TEST(ImmediateInterpreterTest, SemiMtActiveAreaTest) {
  ImmediateInterpreter ii(NULL, NULL);

  HardwareProperties old_hwprops = {
    0,  // left edge
    0,  // top edge
    90.404251,  // right edge
    48.953846,  // bottom edge
    1,  // pixels/TP width
    1,  // pixels/TP height
    133,  // screen DPI x
    133,  // screen DPI y
    -1,  // orientation minimum
    2,   // orientation maximum
    2,  // max fingers
    3,  // max touch
    0,  // t5r2
    1,  // semi-mt
    true,  // is button pad
    0   // has_wheel
  };

  const unsigned kNonPalmFlags = GESTURES_FINGER_WARP_X |
      GESTURES_FINGER_WARP_Y;
  const unsigned kPalmFlags = kNonPalmFlags | GESTURES_FINGER_PALM;

  FingerState old_finger_states[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID, flags
    { 0, 0, 0, 0, 14, 0, 50, 8, 11, kPalmFlags },
    { 0, 0, 0, 0, 33, 0, 50, 8, 11, kPalmFlags },
    { 0, 0, 0, 0, 37, 0, 50, 8, 11, kPalmFlags },
    { 0, 0, 0, 0, 39, 0, 50, 8, 11, kPalmFlags },
  };

  HardwareState old_hardware_states[] = {
    // time, buttons down, finger count, touch count, finger states pointer
    { 0.05, 0, 1, 1, &old_finger_states[0], 0, 0, 0, 0 },
    { 0.10, 0, 1, 1, &old_finger_states[1], 0, 0, 0, 0 },
    { 0.15, 0, 1, 1, &old_finger_states[2], 0, 0, 0, 0 },
    { 0.20, 0, 1, 1, &old_finger_states[3], 0, 0, 0, 0 },
  };

  TestInterpreterWrapper wrapper(&ii, &old_hwprops);
  ii.tap_enable_.val_ = 1;

  // The finger will not change the tap_to_click_state_ at all.
  for (size_t idx = 0; idx < arraysize(old_hardware_states); ++idx) {
    wrapper.SyncInterpret(&old_hardware_states[idx], NULL);
    EXPECT_EQ(kIdl, ii.tap_to_click_state_);
  }

  HardwareProperties new_hwprops = {
    0,  // left edge
    0,  // top edge
    96.085106,  // right edge
    57.492310,  // bottom edge
    1,  // pixels/TP width
    1,  // pixels/TP height
    133,  // screen DPI x
    133,  // screen DPI y
    -1,  // orientation minimum
    2,   // orientation maximum
    2,  // max fingers
    3,  // max touch
    0,  // t5r2
    1,  // semi-mt
    true,  // is button pad
    0   // has_wheel
  };

  FingerState new_finger_states[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID, flags
    { 0, 0, 0, 0, 14, 0, 55, 12, 7, kNonPalmFlags },
    { 0, 0, 0, 0, 33, 0, 55, 12, 7, kNonPalmFlags },
    { 0, 0, 0, 0, 37, 0, 55, 12, 7, kNonPalmFlags },
    { 0, 0, 0, 0, 39, 0, 55, 12, 7, kNonPalmFlags },
  };

  HardwareState new_hardware_states[] = {
    // time, buttons down, finger count, touch count, finger states pointer
    { 0.05, 0, 1, 1, &new_finger_states[0], 0, 0, 0, 0 },
    { 0.10, 0, 1, 1, &new_finger_states[1], 0, 0, 0, 0 },
    { 0.15, 0, 1, 1, &new_finger_states[2], 0, 0, 0, 0 },
    { 0.20, 0, 1, 1, &new_finger_states[3], 0, 0, 0, 0 },
  };

  wrapper.Reset(&ii, &new_hwprops);
  ii.tap_enable_.val_ = true;

  // With new active area, the finger changes the tap_to_click_state_ to
  // FirstTapBegan.
  for (size_t idx = 0; idx < arraysize(new_hardware_states); ++idx) {
    wrapper.SyncInterpret(&new_hardware_states[idx], NULL);
    EXPECT_EQ(ii.kTtcFirstTapBegan, ii.tap_to_click_state_);
  }
}

TEST(ImmediateInterpreterTest, SemiMtNoPinchTest) {
  ImmediateInterpreter ii(NULL, NULL);
  ii.pinch_enable_.val_ = 1;

  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    90.404251,  // right edge
    48.953846,  // bottom edge
    1,  // pixels/TP width
    1,  // pixels/TP height
    133,  // screen DPI x
    133,  // screen DPI y
    -1,  // orientation minimum
    2,   // orientation maximum
    2,  // max fingers
    3,  // max touch
    0,  // t5r2
    0,  // semi-mt
    true,  // is button pad
    0   // has_wheel
  };

  FingerState finger_state[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID, flags
    {0, 0, 0, 0, 25, 0, 30, 53, 1, 0},  // index 0
    {0, 0, 0, 0, 46, 0, 30, 53, 1, 0},  // index 1
    {0, 0, 0, 0, 69, 0, 30, 53, 1, 0},  // index 2

    {0, 0, 0, 0, 67, 0, 30, 53, 1, 0},  // index 3
    {0, 0, 0, 0, 67, 0, 68, 27, 2, 0},

    {0, 0, 0, 0, 91, 0, 43, 52, 1, 0},  // index 5
    {0, 0, 0, 0, 91, 0, 44, 23, 2, 0},

    {0, 0, 0, 0, 91, 0, 43, 52, 1, 0},  // index 7
    {0, 0, 0, 0, 91, 0, 43, 23, 2, 0},
  };


  HardwareState hardware_states[] = {
    // time, buttons down, finger count, touch count, finger states pointer
    { 2106.273252, 0, 1, 1, &finger_state[0], 0, 0, 0, 0 },
    { 2106.285466, 0, 1, 1, &finger_state[1], 0, 0, 0, 0 },
    { 2106.298021, 0, 1, 1, &finger_state[2], 0, 0, 0, 0 },
    { 2106.325599, 0, 2, 2, &finger_state[3], 0, 0, 0, 0 },
    { 2106.648152, 0, 2, 2, &finger_state[5], 0, 0, 0, 0 },
    { 2106.660447, 0, 2, 2, &finger_state[7], 0, 0, 0, 0 },
    // pinch if not semi_mt device
  };

  TestInterpreterWrapper wrapper(&ii, &hwprops);

  Gesture *gesture;
  for (size_t idx = 0; idx < arraysize(hardware_states); ++idx) {
    gesture = wrapper.SyncInterpret(&hardware_states[idx], NULL);
    // reset finger flags
    for (size_t fidx = 0; fidx < hardware_states[idx].finger_cnt; ++fidx)
      hardware_states[idx].fingers[fidx].flags = 0;
  }
  if (gesture)
    EXPECT_EQ(gesture->type, kGestureTypePinch);

  // For a semi_mt device, replay the same inputs should not generate
  // a pinch gesture.
  hwprops.support_semi_mt = 1;
  wrapper.Reset(&ii, &hwprops);
  for (size_t idx = 0; idx < arraysize(hardware_states); ++idx) {
    gesture = wrapper.SyncInterpret(&hardware_states[idx], NULL);
    // reset finger flags
    for (size_t fidx = 0; fidx < hardware_states[idx].finger_cnt; ++fidx)
      hardware_states[idx].fingers[fidx].flags = 0;
  }
  if (gesture)
    EXPECT_NE(gesture->type, kGestureTypePinch);
}

TEST(ImmediateInterpreterTest, WarpedFingersTappingTest) {
  ImmediateInterpreter ii(NULL, NULL);

  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    90.404251,  // right edge
    48.953846,  // bottom edge
    1,  // pixels/TP width
    1,  // pixels/TP height
    133,  // screen DPI x
    133,  // screen DPI y
    -1,  // orientation minimum
    2,   // orientation maximum
    2,  // max fingers
    3,  // max touch
    0,  // t5r2
    1,  // semi-mt
    true,  // is button pad
    0   // has_wheel
  };

  unsigned flags = GESTURES_FINGER_WARP_X_NON_MOVE |
      GESTURES_FINGER_WARP_Y_NON_MOVE |
      GESTURES_FINGER_WARP_X_TAP_MOVE |
      GESTURES_FINGER_WARP_Y_TAP_MOVE;

  FingerState finger_state[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID, flags
    { 0, 0, 0, 0, 42.139996, 0, 46.106384, 39.800003, 0, flags },  // index 0
    { 0, 0, 0, 0, 42.139996, 0, 69.106384, 28.461538, 1, flags },

    // The finger 0 moves more than default threshold 2.0 in Y, but it should
    // still generate final right-click gesture as the WARP flag is set.
    { 0, 0, 0, 0, 43.350002, 0, 45.425529, 36.353844, 0, flags },  // index 2
    { 0, 0, 0, 0, 43.350002, 0, 69.063828, 28.507692, 1, flags },

    { 0, 0, 0, 0, 43.350002, 0, 69.085106, 28.307692, 1, flags },  // index 4
  };

  HardwareState hardware_states[] = {
    // time, buttons down, finger count, touch count, finger states pointer
    { 3897.124791, 0, 2, 2, &finger_state[0], 0, 0, 0, 0 },
    { 3897.136733, 0, 2, 2, &finger_state[2], 0, 0, 0, 0 },
    { 3897.148675, 0, 1, 1, &finger_state[4], 0, 0, 0, 0 },
    { 3897.160675, 0, 0, 0, NULL, 0, 0, 0, 0 },
  };

  ii.tap_enable_.val_ = 1;
  TestInterpreterWrapper wrapper(&ii, &hwprops);

  Gesture *gesture;
  for (size_t idx = 0; idx < arraysize(hardware_states); ++idx)
    gesture = wrapper.SyncInterpret(&hardware_states[idx], NULL);

  ASSERT_NE(gesture, static_cast<Gesture*>(NULL));
  EXPECT_EQ(gesture->type, kGestureTypeButtonsChange);
}

// Test that fling_buffer_depth controls the number of scroll samples to use
// to compute fling.
TEST(ImmediateInterpreterTest, FlingDepthTest) {
  ImmediateInterpreter ii(NULL, NULL);
  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    100,  // right edge
    100,  // bottom edge
    1,  // pixels/TP width
    1,  // pixels/TP height
    96,  // x screen DPI
    96,  // y screen DPI
    -1,  // orientation minimum
    2,   // orientation maximum
    2,  // max fingers
    5,  // max touch
    0,  // tripletap
    1,  // semi-mt
    1,  // is button pad
    0   // has_wheel
  };

  FingerState finger_states[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID
    // Consistent movement for 6 frames
    {0, 0, 0, 0, 20, 0, 40, 20, 1, 0}, // 0
    {0, 0, 0, 0, 20, 0, 60, 20, 2, 0},

    {0, 0, 0, 0, 20, 0, 40, 25, 1, 0}, // 2
    {0, 0, 0, 0, 20, 0, 60, 25, 2, 0},

    {0, 0, 0, 0, 20, 0, 40, 30, 1, 0}, // 4
    {0, 0, 0, 0, 20, 0, 60, 30, 2, 0},

    {0, 0, 0, 0, 20, 0, 40, 35, 1, 0}, // 6
    {0, 0, 0, 0, 20, 0, 60, 35, 2, 0},

    {0, 0, 0, 0, 20, 0, 40, 40, 1, 0}, // 8
    {0, 0, 0, 0, 20, 0, 60, 40, 2, 0},

    {0, 0, 0, 0, 20, 0, 40, 45, 1, 0}, // 10
    {0, 0, 0, 0, 20, 0, 60, 45, 2, 0},

    {0, 0, 0, 0, 20, 0, 40, 50, 1, 0}, // 12
    {0, 0, 0, 0, 20, 0, 60, 50, 2, 0},

    {0, 0, 0, 0, 20, 0, 40, 55, 1, 0}, // 14
    {0, 0, 0, 0, 20, 0, 60, 55, 2, 0},
  };
  HardwareState hardware_states[] = {
    // time, buttons, finger count, touch count, finger states pointer
    { 1.00, 0, 2, 2, &finger_states[0], 0, 0, 0, 0 },
    { 1.01, 0, 2, 2, &finger_states[2], 0, 0, 0, 0 },
    { 1.02, 0, 2, 2, &finger_states[4], 0, 0, 0, 0 },
    { 1.03, 0, 2, 2, &finger_states[6], 0, 0, 0, 0 },
    { 1.04, 0, 2, 2, &finger_states[8], 0, 0, 0, 0 },
    { 1.05, 0, 2, 2, &finger_states[10], 0, 0, 0, 0 },
    { 1.06, 0, 2, 2, &finger_states[12], 0, 0, 0, 0 },
    { 1.07, 0, 2, 2, &finger_states[14], 0, 0, 0, 0 },
  };

  TestInterpreterWrapper wrapper(&ii, &hwprops);

  ii.scroll_manager_.fling_buffer_depth_.val_ = 6;
  size_t fling_buffer_depth =
    (size_t)ii.scroll_manager_.fling_buffer_depth_.val_;

  // The unittest is only meaningful if there are enough hwstates
  ASSERT_GT(arraysize(hardware_states) - 1, fling_buffer_depth)
      << "Hardware state list must be > fling buffer depth + 1";

  // Fill scroll buffer with a set of scrolls
  ii.scroll_buffer_.Clear();
  const HardwareState* prev_hs = NULL;
  for (size_t idx = 0; idx < arraysize(hardware_states); ++idx) {
    const HardwareState* hs = &hardware_states[idx];
    if (prev_hs != NULL) {
      // Cheating here, only using first finger to compute scroll
      const FingerState* fs = &hs->fingers[0];
      const FingerState* prev_fs = prev_hs->GetFingerState(fs->tracking_id);
      EXPECT_NE(reinterpret_cast<const FingerState*>(NULL), prev_fs);
      float dx = fs->position_x - prev_fs->position_x;
      float dy = fs->position_y - prev_fs->position_y;
      float dt = hs->timestamp - prev_hs->timestamp;
      ii.scroll_buffer_.Insert(dx, dy, dt);
      // Enforce assumption that all scrolls are positive in Y only
      EXPECT_DOUBLE_EQ(dx, 0);
      EXPECT_GT(dy, 0);
      EXPECT_GT(dt, 0);
      size_t expected_fling_events = std::min(idx, fling_buffer_depth);
      EXPECT_EQ(ii.scroll_manager_.ScrollEventsForFlingCount(ii.scroll_buffer_),
                expected_fling_events);
    }
    prev_hs = hs;
  }
}

TEST(ImmediateInterpreterTest, ScrollResetTapTest) {
  ImmediateInterpreter ii(NULL, NULL);

  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    96.085106,  // right edge
    57.492310,  // bottom edge
    1,  // pixels/TP width
    1,  // pixels/TP height
    25.4,  // screen DPI x
    25.4,  // screen DPI y
    -1,  // orientation minimum
    2,   // orientation maximum
    2,  // max fingers
    3,  // max touch
    0,  // t5r2
    1,  // semi-mt
    1,  // is button pad
    0   // has_wheel
  };

  FingerState finger_state[] = {
    // TM, Tm, WM, Wm, Press, Orientation, X, Y, TrID, flags
    { 0, 0, 0, 0, 71.180000, 0, 58.446808, 24.000002, 0, 3 },  // index 0
    { 0, 0, 0, 0, 71.180000, 0, 75.042549, 23.676924, 1, 3 },

    { 0, 0, 0, 0, 82.070000, 0, 55.276596, 23.492308, 0, 3 },  // index 2
    { 0, 0, 0, 0, 82.070000, 0, 70.361702, 23.015387, 1, 3 },

    { 0, 0, 0, 0, 76.625000, 0, 58.542553, 23.030769, 0, 3 },  // index 4
    { 0, 0, 0, 0, 76.625000, 0, 59.127659, 22.500002, 1, 1 },

    // prev_result will be scroll, we expect the tap state will be idle
    // after the sample is processed.
    { 0, 0, 0, 0, 71.180000, 0, 61.808510, 22.569231, 0, 3 },  // index 6
    { 0, 0, 0, 0, 71.180000, 0, 47.893616, 21.984617, 1, 1 },

    { 0, 0, 0, 0, 16.730000, 0, 57.617020, 20.830770, 0, 3 },  // index 8
  };

  HardwareState hardware_states[] = {
    // time, buttons down, finger count, touch count, finger states pointer
    { 1296.498245, 0, 2, 2, &finger_state[0], 0, 0, 0, 0 },
    { 1296.510735, 0, 2, 2, &finger_state[2], 0, 0, 0, 0 },
    { 1296.523224, 0, 2, 2, &finger_state[4], 0, 0, 0, 0 },
    { 1296.535753, 0, 2, 2, &finger_state[6], 0, 0, 0, 0 },
    { 1296.548282, 0, 1, 1, &finger_state[8], 0, 0, 0, 0 },
  };

  // SemiMt-specific properties
  ii.tapping_finger_min_separation_.val_ = 0.0;

  ii.tap_enable_.val_ = 1;
  TestInterpreterWrapper wrapper(&ii, &hwprops);

  for (size_t idx = 0; idx < arraysize(hardware_states); ++idx) {
    Gesture* gs = wrapper.SyncInterpret(&hardware_states[idx], NULL);
    if (gs != NULL) {
      if (idx == 2)
        EXPECT_EQ(kGestureTypeScroll, gs->type);
      else
        EXPECT_NE(kGestureTypeButtonsChange, gs->type);
    }
    if (idx >= 3)
      EXPECT_EQ(kIdl, ii.tap_to_click_state_);
  }
}

TEST(ImmediateInterpreterTest, BasicButtonTest) {
  ImmediateInterpreter ii(NULL, NULL);

  HardwareProperties hwprops = {
    0,  // left edge
    0,  // top edge
    96.085106,  // right edge
    57.492310,  // bottom edge
    1,  // pixels/TP width
    1,  // pixels/TP height
    25.4,  // screen DPI x
    25.4,  // screen DPI y
    -1,  // orientation minimum
    2,   // orientation maximum
    2,  // max fingers
    3,  // max touch
    0,  // t5r2
    1,  // semi-mt
    1,  // is button pad
    0   // has_wheel
  };

  HardwareState hardware_states[] = {
    // time, buttons down, finger count, touch count, finger states pointer
    { 0.1, 0, 0, 0, NULL, 0, 0, 0, 0 },
    { 0.3, GESTURES_BUTTON_LEFT, 0, 0, NULL, 0, 0, 0, 0 },   // delay left
    // button down
    { 0.5, GESTURES_BUTTON_LEFT, 0, 0, NULL, 0, 0, 0, 0 },
    { 0.9, 0, 0, 0, NULL, 0, 0, 0, 0 },                      // left button up
    { 1.1, GESTURES_BUTTON_RIGHT, 0, 0, NULL, 0, 0, 0, 0 },  // delay right
    // button down
    { 1.3, GESTURES_BUTTON_RIGHT, 0, 0, NULL, 0, 0, 0, 0 },
    { 1.5, 0, 0, 0, NULL, 0, 0, 0, 0 },                      // right button up
    { 1.6, GESTURES_BUTTON_LEFT, 0, 0, NULL, 0, 0, 0, 0 },   // left button down
    { 1.7, 0, 0, 0, NULL, 0, 0, 0, 0 },  // short left button up (<.3s)
  };

  TestInterpreterWrapper wrapper(&ii, &hwprops);

  for (size_t idx = 0; idx < arraysize(hardware_states); ++idx) {
    Gesture* gs = wrapper.SyncInterpret(&hardware_states[idx], NULL);
    if (idx < 2 || idx == 4 || idx == 7) {
      EXPECT_EQ(NULL, gs);
    } else {
      EXPECT_TRUE(gs != NULL);
      if (gs == NULL)  /* Avoid SEGV on test failure */
        continue;
      EXPECT_EQ(kGestureTypeButtonsChange, gs->type);
      if (idx == 2)
        EXPECT_EQ(GESTURES_BUTTON_LEFT, gs->details.buttons.down);
      else if (idx == 3)
        EXPECT_EQ(GESTURES_BUTTON_LEFT, gs->details.buttons.up);
      else if (idx == 5)
        EXPECT_EQ(GESTURES_BUTTON_RIGHT, gs->details.buttons.down);
      else if (idx == 6)
        EXPECT_EQ(GESTURES_BUTTON_RIGHT, gs->details.buttons.up);
      else if (idx == 8) {
        EXPECT_EQ(GESTURES_BUTTON_LEFT, gs->details.buttons.up);
        EXPECT_EQ(GESTURES_BUTTON_LEFT, gs->details.buttons.down);
      }
    }
  }
}
}  // namespace gestures
