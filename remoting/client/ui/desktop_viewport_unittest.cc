// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/client/ui/desktop_viewport.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/strings/stringprintf.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace remoting {

namespace {

const float EPSILON = 0.001f;

}  // namespace

class DesktopViewportTest : public testing::Test {
 public:
  void SetUp() override;
  void TearDown() override;

 protected:
  void AssertTransformationReceived(const base::Location& from_here,
                                    float scale,
                                    float offset_x,
                                    float offset_y);

  ViewMatrix ReleaseReceivedTransformation();

  DesktopViewport viewport_;

 private:
  void OnTransformationChanged(const ViewMatrix& matrix);

  ViewMatrix received_transformation_;
};

void DesktopViewportTest::SetUp() {
  viewport_.RegisterOnTransformationChangedCallback(
      base::Bind(&DesktopViewportTest::OnTransformationChanged,
                 base::Unretained(this)),
      true);
}

void DesktopViewportTest::TearDown() {
  ASSERT_TRUE(received_transformation_.IsEmpty());
}

void DesktopViewportTest::AssertTransformationReceived(
    const base::Location& from_here,
    float scale,
    float offset_x,
    float offset_y) {
  ASSERT_FALSE(received_transformation_.IsEmpty())
      << "Matrix has not been received yet."
      << "Location: " << from_here.ToString();
  ViewMatrix expected(scale, {offset_x, offset_y});
  std::array<float, 9> expected_array = expected.ToMatrixArray();
  std::array<float, 9> actual_array = received_transformation_.ToMatrixArray();

  for (int i = 0; i < 9; i++) {
    float diff = expected_array[i] - actual_array[i];
    ASSERT_TRUE(diff > -EPSILON && diff < EPSILON)
        << "Matrix doesn't match. \n"
        << base::StringPrintf("Expected scale: %f, offset: (%f, %f)\n",
                              expected_array[0], expected_array[2],
                              expected_array[5])
        << base::StringPrintf("Actual scale: %f, offset: (%f, %f)\n",
                              actual_array[0], actual_array[2], actual_array[5])
        << "Location: " << from_here.ToString();
  }

  received_transformation_ = ViewMatrix();
}

ViewMatrix DesktopViewportTest::ReleaseReceivedTransformation() {
  EXPECT_FALSE(received_transformation_.IsEmpty());
  ViewMatrix out = received_transformation_;
  received_transformation_ = ViewMatrix();
  return out;
}

void DesktopViewportTest::OnTransformationChanged(const ViewMatrix& matrix) {
  ASSERT_TRUE(received_transformation_.IsEmpty())
      << "Previous matrix has not been asserted.";
  received_transformation_ = matrix;
}

TEST_F(DesktopViewportTest, TestViewportInitialization1) {
  // VP < DP. Desktop shrinks to fit.
  // +====+------+
  // | VP | DP   |
  // |    |      |
  // +====+------+
  viewport_.SetDesktopSize(8, 6);
  viewport_.SetSurfaceSize(2, 3);
  AssertTransformationReceived(FROM_HERE, 0.5f, 0.f, 0.f);
}

TEST_F(DesktopViewportTest, TestViewportInitialization2) {
  // VP < DP. Desktop shrinks to fit.
  // +-----------------+
  // |       DP        |
  // |                 |
  // +=================+
  // |       VP        |
  // +=================+
  viewport_.SetDesktopSize(8, 6);
  viewport_.SetSurfaceSize(3, 2);
  AssertTransformationReceived(FROM_HERE, 0.375, 0.f, 0.f);
}

TEST_F(DesktopViewportTest, TestViewportInitialization3) {
  // VP < DP. Desktop shrinks to fit.
  // +========+----+
  // |  VP    | DP |
  // +========+----+
  viewport_.SetDesktopSize(9, 3);
  viewport_.SetSurfaceSize(2, 1);
  AssertTransformationReceived(FROM_HERE, 0.333f, 0.f, 0.f);
}

TEST_F(DesktopViewportTest, TestViewportInitialization4) {
  // VP > DP. Desktop grows to fit.
  // +====+------+
  // | VP | DP   |
  // |    |      |
  // +====+------+
  viewport_.SetDesktopSize(2, 1);
  viewport_.SetSurfaceSize(3, 4);
  AssertTransformationReceived(FROM_HERE, 4.f, 0.f, 0.f);
}

TEST_F(DesktopViewportTest, TestMoveDesktop) {
  // +====+------+
  // | VP | DP   |
  // |    |      |
  // +====+------+
  viewport_.SetDesktopSize(8, 6);
  viewport_.SetSurfaceSize(2, 3);
  AssertTransformationReceived(FROM_HERE, 0.5f, 0.f, 0.f);

  // <--- DP
  // +------+====+
  // | DP   | VP |
  // |      |    |
  // +------+====+
  viewport_.MoveDesktop(-2.f, 0.f);
  AssertTransformationReceived(FROM_HERE, 0.5f, -2.f, 0.f);

  //      +====+
  // +----| VP |
  // | DP |    |
  // |    +====+
  // +--------+
  // Bounces back.
  viewport_.MoveDesktop(-1.f, 1.f);
  AssertTransformationReceived(FROM_HERE, 0.5f, -2.f, 0.f);
}

TEST_F(DesktopViewportTest, TestMoveAndScaleDesktop) {
  // Number in surface coordinate.
  //
  // +====+------+
  // | VP | DP   |
  // |    |      | 3
  // +====+------+
  //        4
  viewport_.SetDesktopSize(8, 6);
  viewport_.SetSurfaceSize(2, 3);
  AssertTransformationReceived(FROM_HERE, 0.5f, 0.f, 0.f);

  // Scale at pivot point (2, 3) by 1.5x.
  // +------------------+
  // |                  |
  // |   +====+   DP    | 4.5
  // |   | VP |         |
  // |   |    |         |
  // +---+====+---------+
  //       2     6
  viewport_.ScaleDesktop(2.f, 3.f, 1.5f);
  AssertTransformationReceived(FROM_HERE, 0.75f, -1.f, -1.5f);

  // Move VP to the top-right.
  // +-------------+====+
  // |             | VP |
  // |     DP      |    |
  // |             +====+ 4.5
  // |               2  |
  // +------------------+
  //         6
  viewport_.MoveDesktop(-10000.f, 10000.f);
  AssertTransformationReceived(FROM_HERE, 0.75f, -4.f, 0.f);

  // Scale at (2, 0) by 0.5x.
  //      VP
  //       +====+
  //    +--+----+
  // DP |  |    |
  //    +--+----+
  //       +====+
  viewport_.ScaleDesktop(2.f, 0.f, 0.5f);
  AssertTransformationReceived(FROM_HERE, 0.375, -1.f, 0.375f);

  // Scale all the way down.
  // +========+
  // |   VP   |
  // +--------+
  // |   DP   |
  // +--------+
  // |        |
  // +========+
  viewport_.ScaleDesktop(20.f, 0.f, 0.0001f);
  AssertTransformationReceived(FROM_HERE, 0.25f, 0.f, 0.75f);
}

TEST_F(DesktopViewportTest, TestSetViewportCenter) {
  // Numbers in desktop coordinates.
  //
  // +====+------+
  // | VP | DP   |
  // |    |      | 6
  // +====+------+
  //        8
  viewport_.SetDesktopSize(8, 6);
  viewport_.SetSurfaceSize(2, 3);
  AssertTransformationReceived(FROM_HERE, 0.5f, 0.f, 0.f);

  //  1.6
  // +==+--------+
  // |VP|2.4  DP |
  // +==+        | 6
  // +-----------+
  //       8
  viewport_.ScaleDesktop(0.f, 0.f, 2.5f);
  AssertTransformationReceived(FROM_HERE, 1.25f, 0.f, 0.f);

  // Move VP to center of the desktop.
  // +------------------+
  // |      +1.6=+      |
  // |      | VP |2.4   | 6
  // |      +====+      |
  // +------------------+
  //          8
  viewport_.SetViewportCenter(4.f, 3.f);
  AssertTransformationReceived(FROM_HERE, 1.25f, -4.f, -2.25f);

  // Move it out of bound and bounce it back.
  // +------------------+
  // |                  |
  // |     DP           |
  // |               +====+
  // |               | VP |
  // +---------------|    |
  //                 +====+
  viewport_.SetViewportCenter(1000.f, 1000.f);
  AssertTransformationReceived(FROM_HERE, 1.25f, -8.f, -4.5f);
}

TEST_F(DesktopViewportTest, TestScaleDesktop) {
  // Number in surface coordinate.
  //
  // +====+------+
  // | VP | DP   |
  // |    |      | 3
  // +====+------+
  //        4
  viewport_.SetDesktopSize(8, 6);
  viewport_.SetSurfaceSize(2, 3);
  AssertTransformationReceived(FROM_HERE, 0.5f, 0.f, 0.f);

  ViewMatrix old_transformation(0.5f, {0.f, 0.f});

  ViewMatrix::Point surface_point = old_transformation.MapPoint({1.2f, 1.3f});

  // Scale a little bit at a pivot point.
  viewport_.ScaleDesktop(surface_point.x, surface_point.y, 1.1f);

  ViewMatrix new_transformation = ReleaseReceivedTransformation();

  // Verify the pivot point is fixed.
  ViewMatrix::Point new_surface_point =
      new_transformation.MapPoint({1.2f, 1.3f});
  ASSERT_FLOAT_EQ(surface_point.x, new_surface_point.x);
  ASSERT_FLOAT_EQ(surface_point.y, new_surface_point.y);

  // Verify the scale is correct.
  ASSERT_FLOAT_EQ(old_transformation.GetScale() * 1.1f,
                  new_transformation.GetScale());
}

}  // namespace remoting
