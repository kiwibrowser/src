// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/input/scroll_snap_data.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace cc {

class ScrollSnapDataTest : public testing::Test {};

TEST_F(ScrollSnapDataTest, FindsClosestSnapPositionIndependently) {
  SnapContainerData data(
      ScrollSnapType(false, SnapAxis::kBoth, SnapStrictness::kMandatory),
      gfx::ScrollOffset(360, 380));
  gfx::ScrollOffset current_position(100, 100);
  SnapAreaData snap_x_only(
      SnapAxis::kX, gfx::ScrollOffset(80, SnapAreaData::kInvalidScrollPosition),
      gfx::RectF(0, 0, 360, 380), false);
  SnapAreaData snap_y_only(
      SnapAxis::kY, gfx::ScrollOffset(SnapAreaData::kInvalidScrollPosition, 70),
      gfx::RectF(0, 0, 360, 380), false);
  SnapAreaData snap_on_both(SnapAxis::kBoth, gfx::ScrollOffset(50, 150),
                            gfx::RectF(0, 0, 360, 380), false);
  data.AddSnapAreaData(snap_x_only);
  data.AddSnapAreaData(snap_y_only);
  data.AddSnapAreaData(snap_on_both);
  gfx::ScrollOffset snap_position;
  EXPECT_TRUE(
      data.FindSnapPosition(current_position, true, true, &snap_position));
  EXPECT_EQ(80, snap_position.x());
  EXPECT_EQ(70, snap_position.y());
}

TEST_F(ScrollSnapDataTest, FindsClosestSnapPositionOnAxisValueBoth) {
  SnapContainerData data(
      ScrollSnapType(false, SnapAxis::kBoth, SnapStrictness::kMandatory),
      gfx::ScrollOffset(360, 380));
  gfx::ScrollOffset current_position(40, 150);
  SnapAreaData snap_x_only(
      SnapAxis::kX, gfx::ScrollOffset(80, SnapAreaData::kInvalidScrollPosition),
      gfx::RectF(0, 0, 360, 380), false);
  SnapAreaData snap_y_only(
      SnapAxis::kY, gfx::ScrollOffset(SnapAreaData::kInvalidScrollPosition, 70),
      gfx::RectF(0, 0, 360, 380), false);
  SnapAreaData snap_on_both(SnapAxis::kBoth, gfx::ScrollOffset(50, 150),
                            gfx::RectF(0, 0, 360, 380), false);
  data.AddSnapAreaData(snap_x_only);
  data.AddSnapAreaData(snap_y_only);
  data.AddSnapAreaData(snap_on_both);
  gfx::ScrollOffset snap_position;
  EXPECT_TRUE(
      data.FindSnapPosition(current_position, true, true, &snap_position));
  EXPECT_EQ(50, snap_position.x());
  EXPECT_EQ(150, snap_position.y());
}

TEST_F(ScrollSnapDataTest, DoesNotSnapOnNonScrolledAxis) {
  SnapContainerData data(
      ScrollSnapType(false, SnapAxis::kBoth, SnapStrictness::kMandatory),
      gfx::ScrollOffset(360, 380));
  gfx::ScrollOffset current_position(100, 100);
  SnapAreaData snap_x_only(
      SnapAxis::kX, gfx::ScrollOffset(80, SnapAreaData::kInvalidScrollPosition),
      gfx::RectF(0, 0, 360, 380), false);
  SnapAreaData snap_y_only(
      SnapAxis::kY, gfx::ScrollOffset(SnapAreaData::kInvalidScrollPosition, 70),
      gfx::RectF(0, 0, 360, 380), false);
  data.AddSnapAreaData(snap_x_only);
  data.AddSnapAreaData(snap_y_only);
  gfx::ScrollOffset snap_position;
  EXPECT_TRUE(
      data.FindSnapPosition(current_position, true, false, &snap_position));
  EXPECT_EQ(80, snap_position.x());
  EXPECT_EQ(100, snap_position.y());
}

TEST_F(ScrollSnapDataTest, DoesNotSnapOnNonVisibleAreas) {
  SnapContainerData data(
      ScrollSnapType(false, SnapAxis::kBoth, SnapStrictness::kMandatory),
      gfx::ScrollOffset(360, 380));
  gfx::ScrollOffset current_position(100, 100);
  SnapAreaData non_visible_x(SnapAxis::kBoth, gfx::ScrollOffset(70, 70),
                             gfx::RectF(0, 0, 90, 200), false);
  SnapAreaData non_visible_y(SnapAxis::kBoth, gfx::ScrollOffset(70, 70),
                             gfx::RectF(0, 0, 200, 90), false);
  data.AddSnapAreaData(non_visible_x);
  data.AddSnapAreaData(non_visible_y);
  gfx::ScrollOffset snap_position;
  EXPECT_FALSE(
      data.FindSnapPosition(current_position, true, true, &snap_position));
}

TEST_F(ScrollSnapDataTest, SnapOnClosestAxisFirstIfVisibilityConflicts) {
  SnapContainerData data(
      ScrollSnapType(false, SnapAxis::kBoth, SnapStrictness::kMandatory),
      gfx::ScrollOffset(360, 380));
  gfx::ScrollOffset current_position(100, 100);

  // Both the areas are currently visible.
  // However, if we snap to them on x and y independently, none is visible after
  // snapping. So we only snap on the axis that has a closer snap point first.
  // After that, we look for another snap point on y axis which does not
  // conflict with the snap point on x.
  SnapAreaData snap_x(
      SnapAxis::kX, gfx::ScrollOffset(80, SnapAreaData::kInvalidScrollPosition),
      gfx::RectF(60, 60, 60, 60), false);
  SnapAreaData snap_y1(
      SnapAxis::kY,
      gfx::ScrollOffset(SnapAreaData::kInvalidScrollPosition, 130),
      gfx::RectF(90, 90, 60, 60), false);
  SnapAreaData snap_y2(
      SnapAxis::kY, gfx::ScrollOffset(SnapAreaData::kInvalidScrollPosition, 60),
      gfx::RectF(50, 50, 60, 60), false);
  data.AddSnapAreaData(snap_x);
  data.AddSnapAreaData(snap_y1);
  data.AddSnapAreaData(snap_y2);
  gfx::ScrollOffset snap_position;
  EXPECT_TRUE(
      data.FindSnapPosition(current_position, true, true, &snap_position));
  EXPECT_EQ(80, snap_position.x());
  EXPECT_EQ(60, snap_position.y());
}

TEST_F(ScrollSnapDataTest, DoesNotSnapToPositionsOutsideProximityRange) {
  SnapContainerData data(
      ScrollSnapType(false, SnapAxis::kBoth, SnapStrictness::kMandatory),
      gfx::ScrollOffset(360, 380));
  data.set_proximity_range(gfx::ScrollOffset(50, 50));

  gfx::ScrollOffset current_position(100, 100);
  SnapAreaData area(SnapAxis::kBoth, gfx::ScrollOffset(80, 160),
                    gfx::RectF(50, 50, 200, 200), false);
  data.AddSnapAreaData(area);
  gfx::ScrollOffset snap_position;
  EXPECT_TRUE(
      data.FindSnapPosition(current_position, true, true, &snap_position));

  // The snap position on x, 80, is within the proximity range of [50, 150].
  // However, the snap position on y, 160, is outside the proximity range of
  // [50, 150], so we should only snap on x.
  EXPECT_EQ(80, snap_position.x());
  EXPECT_EQ(100, snap_position.y());
}

}  // namespace cc
