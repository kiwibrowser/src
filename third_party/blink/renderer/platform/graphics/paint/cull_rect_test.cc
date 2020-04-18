// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/paint/cull_rect.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/geometry/float_rect.h"
#include "third_party/blink/renderer/platform/geometry/layout_rect.h"
#include "third_party/blink/renderer/platform/testing/runtime_enabled_features_test_helpers.h"

namespace blink {

class CullRectTest : public testing::Test {
 protected:
  IntRect Rect(const CullRect& cull_rect) { return cull_rect.rect_; }
};

TEST_F(CullRectTest, IntersectsCullRect) {
  CullRect cull_rect(IntRect(0, 0, 50, 50));

  EXPECT_TRUE(cull_rect.IntersectsCullRect(IntRect(0, 0, 1, 1)));
  EXPECT_FALSE(cull_rect.IntersectsCullRect(IntRect(51, 51, 1, 1)));
}

TEST_F(CullRectTest, IntersectsCullRectWithLayoutRect) {
  CullRect cull_rect(IntRect(0, 0, 50, 50));

  EXPECT_TRUE(cull_rect.IntersectsCullRect(LayoutRect(0, 0, 1, 1)));
  EXPECT_TRUE(cull_rect.IntersectsCullRect(LayoutRect(
      LayoutUnit(0.1), LayoutUnit(0.1), LayoutUnit(0.1), LayoutUnit(0.1))));
}

TEST_F(CullRectTest, IntersectsCullRectWithTransform) {
  CullRect cull_rect(IntRect(0, 0, 50, 50));
  AffineTransform transform;
  transform.Translate(-2, -2);

  EXPECT_TRUE(cull_rect.IntersectsCullRect(transform, IntRect(51, 51, 1, 1)));
  EXPECT_FALSE(cull_rect.IntersectsCullRect(IntRect(52, 52, 1, 1)));
}

TEST_F(CullRectTest, UpdateCullRect) {
  CullRect cull_rect(IntRect(1, 1, 50, 50));
  AffineTransform transform;
  transform.Translate(1, 1);
  cull_rect.UpdateCullRect(transform);

  EXPECT_EQ(IntRect(0, 0, 50, 50), Rect(cull_rect));
}

TEST_F(CullRectTest, IntersectsVerticalRange) {
  CullRect cull_rect(IntRect(0, 0, 50, 100));

  EXPECT_TRUE(cull_rect.IntersectsVerticalRange(LayoutUnit(), LayoutUnit(1)));
  EXPECT_FALSE(
      cull_rect.IntersectsVerticalRange(LayoutUnit(100), LayoutUnit(101)));
}

TEST_F(CullRectTest, IntersectsHorizontalRange) {
  CullRect cull_rect(IntRect(0, 0, 50, 100));

  EXPECT_TRUE(cull_rect.IntersectsHorizontalRange(LayoutUnit(), LayoutUnit(1)));
  EXPECT_FALSE(
      cull_rect.IntersectsHorizontalRange(LayoutUnit(50), LayoutUnit(51)));
}

TEST_F(CullRectTest, UpdateForScrollingContents) {
  ScopedSlimmingPaintV2ForTest spv2(true);

  CullRect cull_rect(IntRect(0, 0, 50, 100));
  AffineTransform transform;
  transform.Translate(10, 15);
  cull_rect.UpdateForScrollingContents(IntRect(20, 10, 40, 50), transform);

  // Clipped: (20, 10, 30, 50)
  // Expanded: (-3980, -3990, 8030, 8050)
  // Inverse transformed: (-3990, -4005, 8030, 8050)
  EXPECT_EQ(IntRect(-3990, -4005, 8030, 8050), Rect(cull_rect));
}

}  // namespace blink
