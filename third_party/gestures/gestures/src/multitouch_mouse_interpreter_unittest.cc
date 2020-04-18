// Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>

#include "gestures/include/gestures.h"
#include "gestures/include/multitouch_mouse_interpreter.h"
#include "gestures/include/unittest_util.h"
#include "gestures/include/util.h"

namespace gestures {

class MultitouchMouseInterpreterTest : public ::testing::Test {};

TEST(MultitouchMouseInterpreterTest, SimpleTest) {
  MultitouchMouseInterpreter mi(NULL, NULL);
  Gesture* gs;

  HardwareProperties hwprops = {
    133, 728, 10279, 5822,  // left, top, right, bottom
    (10279.0 - 133.0) / 100.0,  // x res (pixels/mm)
    (5822.0 - 728.0) / 60,  // y res (pixels/mm)
    133, 133,  // scrn DPI X, Y
    -1,  // orientation minimum
    2,   // orientation maximum
    2, 5,  // max fingers, max_touch
    0, 0, 0, 0  //t5r2, semi, button pad
  };
  TestInterpreterWrapper wrapper(&mi, &hwprops);

  FingerState fs_0[] = {
    { 1, 1, 0, 0, 0, 0, 0, 0, 1, 0 },
    { 1, 1, 0, 0, 0, 0, 0, 0, 2, 0 },
  };
  FingerState fs_1[] = {
    { 1, 1, 0, 0, 0, 0, 3, 4, 1, 0 },
    { 1, 1, 0, 0, 0, 0, 6, 8, 2, 0 },
  };
  HardwareState hwstates[] = {
    { 200000, 0, 2, 2, fs_0, 0, 0, 0, 0 },
    { 210000, 0, 2, 2, fs_0, 9, -7, 0, 0 },
    { 220000, 1, 2, 2, fs_0, 0, 0, 0, 0 },
    { 230000, 0, 2, 2, fs_0, 0, 0, 0, 0 },
    { 240000, 0, 2, 2, fs_1, 0, 0, 0, 0 },
  };

  // Make snap impossible
  mi.scroll_manager_.horizontal_scroll_snap_slope_.val_ = 0;
  mi.scroll_manager_.vertical_scroll_snap_slope_.val_ = 100;

  gs = wrapper.SyncInterpret(&hwstates[0], NULL);
  EXPECT_EQ(reinterpret_cast<Gesture*>(NULL), gs);

  gs = wrapper.SyncInterpret(&hwstates[1], NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), gs);
  EXPECT_EQ(kGestureTypeMove, gs->type);
  EXPECT_EQ(9, gs->details.move.dx);
  EXPECT_EQ(-7, gs->details.move.dy);
  EXPECT_EQ(200000, gs->start_time);
  EXPECT_EQ(210000, gs->end_time);

  gs = wrapper.SyncInterpret(&hwstates[2], NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), gs);
  EXPECT_EQ(kGestureTypeButtonsChange, gs->type);
  EXPECT_EQ(1, gs->details.buttons.down);
  EXPECT_EQ(0, gs->details.buttons.up);
  EXPECT_GE(210000, gs->start_time);
  EXPECT_EQ(220000, gs->end_time);

  gs = wrapper.SyncInterpret(&hwstates[3], NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), gs);
  EXPECT_EQ(kGestureTypeButtonsChange, gs->type);
  EXPECT_EQ(0, gs->details.buttons.down);
  EXPECT_EQ(1, gs->details.buttons.up);
  EXPECT_EQ(220000, gs->start_time);
  EXPECT_EQ(230000, gs->end_time);

  gs = wrapper.SyncInterpret(&hwstates[4], NULL);
  ASSERT_NE(reinterpret_cast<Gesture*>(NULL), gs);
  EXPECT_EQ(kGestureTypeScroll, gs->type);
  EXPECT_EQ(6, gs->details.scroll.dx);
  EXPECT_EQ(8, gs->details.scroll.dy);
  EXPECT_EQ(230000, gs->start_time);
  EXPECT_EQ(240000, gs->end_time);
}

}  // namespace gestures
