// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/layers/touch_action_region.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace cc {
namespace {

TEST(TouchActionRegionTest, GetWhiteListedTouchActionMapOverlapToZero) {
  TouchActionRegion touch_action_region;
  touch_action_region.Union(kTouchActionPanLeft, gfx::Rect(0, 0, 50, 50));
  touch_action_region.Union(kTouchActionPanRight, gfx::Rect(25, 25, 25, 25));
  // The point is only in PanLeft, so the result is PanLeft.
  EXPECT_EQ(kTouchActionPanLeft,
            touch_action_region.GetWhiteListedTouchAction(gfx::Point(10, 10)));
  // The point is in both PanLeft and PanRight, and those actions have no
  // common components, so the result is None.
  EXPECT_EQ(kTouchActionNone,
            touch_action_region.GetWhiteListedTouchAction(gfx::Point(30, 30)));
  // The point is in neither PanLeft nor PanRight, so the result is Auto since
  // the default touch action is auto.
  EXPECT_EQ(kTouchActionAuto,
            touch_action_region.GetWhiteListedTouchAction(gfx::Point(60, 60)));
}

TEST(TouchActionRegionTest, GetWhiteListedTouchActionMapOverlapToNonZero) {
  TouchActionRegion touch_action_region;
  touch_action_region.Union(kTouchActionPanX, gfx::Rect(0, 0, 50, 50));
  touch_action_region.Union(kTouchActionPanRight, gfx::Rect(25, 25, 25, 25));
  // The point is only in PanX, so the result is PanX.
  EXPECT_EQ(kTouchActionPanX,
            touch_action_region.GetWhiteListedTouchAction(gfx::Point(10, 10)));
  // The point is in both PanX and PanRight, and PanRight is a common component,
  // so the result is PanRight.
  EXPECT_EQ(kTouchActionPanRight,
            touch_action_region.GetWhiteListedTouchAction(gfx::Point(30, 30)));
  // The point is neither PanX nor PanRight, so the result is Auto since the
  // default touch action is auto.
  EXPECT_EQ(kTouchActionAuto,
            touch_action_region.GetWhiteListedTouchAction(gfx::Point(60, 60)));
}

TEST(TouchActionRegionTest, GetWhiteListedTouchActionEmptyMap) {
  TouchActionRegion touch_action_region;
  // The result is Auto since the map is empty and the default touch
  // action is auto.
  EXPECT_EQ(kTouchActionAuto,
            touch_action_region.GetWhiteListedTouchAction(gfx::Point(10, 10)));
}

TEST(TouchActionRegionTest, GetWhiteListedTouchActionSingleMapEntry) {
  TouchActionRegion touch_action_region;
  touch_action_region.Union(kTouchActionPanUp, gfx::Rect(0, 0, 50, 50));
  // The point is only in PanUp, so the result is PanUp.
  EXPECT_EQ(kTouchActionPanUp,
            touch_action_region.GetWhiteListedTouchAction(gfx::Point(10, 10)));
  // The point is not in PanUp, so the result is Auto since the default touch
  // action is auto.
  EXPECT_EQ(kTouchActionAuto,
            touch_action_region.GetWhiteListedTouchAction(gfx::Point(60, 60)));
}

TEST(TouchActionRegionTest, ConstructorFromMap) {
  gfx::Rect rect1(0, 0, 50, 50);
  gfx::Rect rect2(50, 0, 50, 50);
  base::flat_map<TouchAction, Region> region_map;
  region_map[kTouchActionPanX].Union(rect1);
  region_map[kTouchActionPanUp].Union(rect2);
  TouchActionRegion touch_action_region(region_map);

  Region expected_region1(rect1);
  Region expected_region2(rect2);
  Region expected_region3(rect1);
  expected_region3.Union(rect2);

  EXPECT_EQ(touch_action_region.GetRegionForTouchAction(kTouchActionPanX),
            expected_region1);
  EXPECT_EQ(touch_action_region.GetRegionForTouchAction(kTouchActionPanUp),
            expected_region2);
  EXPECT_EQ(touch_action_region.region(), expected_region3);
}

}  // namespace
}  // namespace cc
