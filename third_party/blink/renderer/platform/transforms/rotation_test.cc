// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/transforms/rotation.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

TEST(RotationTest, GetCommonAxisTest) {
  FloatPoint3D axis;
  double angle_a;
  double angle_b;

  EXPECT_TRUE(Rotation::GetCommonAxis(Rotation(FloatPoint3D(0, 0, 0), 0),
                                      Rotation(FloatPoint3D(1, 2, 3), 100),
                                      axis, angle_a, angle_b));
  EXPECT_EQ(FloatPoint3D(1, 2, 3), axis);
  EXPECT_EQ(0, angle_a);
  EXPECT_EQ(100, angle_b);

  EXPECT_TRUE(Rotation::GetCommonAxis(Rotation(FloatPoint3D(1, 2, 3), 100),
                                      Rotation(FloatPoint3D(0, 0, 0), 0), axis,
                                      angle_a, angle_b));
  EXPECT_EQ(FloatPoint3D(1, 2, 3), axis);
  EXPECT_EQ(100, angle_a);
  EXPECT_EQ(0, angle_b);

  EXPECT_TRUE(Rotation::GetCommonAxis(Rotation(FloatPoint3D(0, 0, 0), 100),
                                      Rotation(FloatPoint3D(1, 2, 3), 100),
                                      axis, angle_a, angle_b));
  EXPECT_EQ(FloatPoint3D(1, 2, 3), axis);
  EXPECT_EQ(0, angle_a);
  EXPECT_EQ(100, angle_b);

  EXPECT_TRUE(Rotation::GetCommonAxis(Rotation(FloatPoint3D(3, 2, 1), 0),
                                      Rotation(FloatPoint3D(1, 2, 3), 100),
                                      axis, angle_a, angle_b));
  EXPECT_EQ(FloatPoint3D(1, 2, 3), axis);
  EXPECT_EQ(0, angle_a);
  EXPECT_EQ(100, angle_b);

  EXPECT_TRUE(Rotation::GetCommonAxis(Rotation(FloatPoint3D(1, 2, 3), 50),
                                      Rotation(FloatPoint3D(1, 2, 3), 100),
                                      axis, angle_a, angle_b));
  EXPECT_EQ(FloatPoint3D(1, 2, 3), axis);
  EXPECT_EQ(50, angle_a);
  EXPECT_EQ(100, angle_b);

  EXPECT_TRUE(Rotation::GetCommonAxis(Rotation(FloatPoint3D(1, 2, 3), 50),
                                      Rotation(FloatPoint3D(2, 4, 6), 100),
                                      axis, angle_a, angle_b));
  EXPECT_EQ(FloatPoint3D(1, 2, 3), axis);
  EXPECT_EQ(50, angle_a);
  EXPECT_EQ(100, angle_b);

  EXPECT_FALSE(Rotation::GetCommonAxis(Rotation(FloatPoint3D(1, 2, 3), 50),
                                       Rotation(FloatPoint3D(3, 2, 1), 100),
                                       axis, angle_a, angle_b));

  EXPECT_FALSE(Rotation::GetCommonAxis(Rotation(FloatPoint3D(1, 2, 3), 50),
                                       Rotation(FloatPoint3D(-1, -2, -3), 100),
                                       axis, angle_a, angle_b));
}

}  // namespace blink
