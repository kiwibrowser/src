// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/input/snap_fling_curve.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace cc {
namespace test {

TEST(SnapFlingCurveTest, CurveInitialization) {
  SnapFlingCurve active_curve(gfx::Vector2dF(100, 100),
                              gfx::Vector2dF(500, 500), base::TimeTicks());
  EXPECT_FALSE(active_curve.IsFinished());

  SnapFlingCurve finished_curve(gfx::Vector2dF(100, 100),
                                gfx::Vector2dF(100, 100), base::TimeTicks());
  EXPECT_TRUE(finished_curve.IsFinished());
}

TEST(SnapFlingCurveTest, AdvanceHalfwayThrough) {
  SnapFlingCurve curve(gfx::Vector2dF(100, 100), gfx::Vector2dF(500, 500),
                       base::TimeTicks());
  base::TimeDelta duration = curve.duration();
  gfx::Vector2dF delta1 =
      curve.GetScrollDelta(base::TimeTicks() + duration / 2);
  EXPECT_LT(0, delta1.x());
  EXPECT_LT(0, delta1.y());
  EXPECT_FALSE(curve.IsFinished());

  // Repeated offset computations at the same timestamp before applying the
  // scrolled delta should yield identical results.
  gfx::Vector2dF delta2 =
      curve.GetScrollDelta(base::TimeTicks() + duration / 2);
  EXPECT_EQ(delta1, delta2);
  EXPECT_FALSE(curve.IsFinished());

  curve.UpdateCurrentOffset(gfx::Vector2dF(100, 100) + delta1);
  EXPECT_FALSE(curve.IsFinished());
}

TEST(SnapFlingCurveTest, AdvanceFullyThroughOnlyFinishesAfterUpdate) {
  SnapFlingCurve curve(gfx::Vector2dF(100, 100), gfx::Vector2dF(500, 500),
                       base::TimeTicks());
  gfx::Vector2dF delta =
      curve.GetScrollDelta(base::TimeTicks() + curve.duration());
  EXPECT_EQ(gfx::Vector2dF(400, 400), delta);
  EXPECT_FALSE(curve.IsFinished());

  curve.UpdateCurrentOffset(gfx::Vector2dF(500, 500));
  EXPECT_TRUE(curve.IsFinished());
}

TEST(SnapFlingCurveTest, ReturnsZeroAfterFinished) {
  SnapFlingCurve curve(gfx::Vector2dF(100, 100), gfx::Vector2dF(500, 500),
                       base::TimeTicks());
  curve.UpdateCurrentOffset(gfx::Vector2dF(500, 500));
  EXPECT_TRUE(curve.IsFinished());

  gfx::Vector2dF delta = curve.GetScrollDelta(base::TimeTicks());
  EXPECT_EQ(gfx::Vector2dF(), delta);
  EXPECT_TRUE(curve.IsFinished());

  delta = curve.GetScrollDelta(base::TimeTicks() + curve.duration());
  EXPECT_EQ(gfx::Vector2dF(), delta);
  EXPECT_TRUE(curve.IsFinished());
}

}  // namespace test
}  // namespace cc
