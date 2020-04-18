// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/geometry/ng_physical_rect.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

namespace {

TEST(NGGeometryUnitsTest, SnapToDevicePixels) {
  NGPhysicalRect rect(NGPhysicalLocation(LayoutUnit(4.5), LayoutUnit(9.5)),
                      NGPhysicalSize(LayoutUnit(10.5), LayoutUnit(11.5)));
  NGPixelSnappedPhysicalRect snapped_rect = rect.SnapToDevicePixels();
  EXPECT_EQ(5, snapped_rect.left);
  EXPECT_EQ(10, snapped_rect.top);
  EXPECT_EQ(10, snapped_rect.width);
  EXPECT_EQ(11, snapped_rect.height);

  rect = NGPhysicalRect(NGPhysicalLocation(LayoutUnit(4), LayoutUnit(9)),
                        NGPhysicalSize(LayoutUnit(10.5), LayoutUnit(11.5)));
  snapped_rect = rect.SnapToDevicePixels();
  EXPECT_EQ(4, snapped_rect.left);
  EXPECT_EQ(9, snapped_rect.top);
  EXPECT_EQ(11, snapped_rect.width);
  EXPECT_EQ(12, snapped_rect.height);

  rect = NGPhysicalRect(NGPhysicalLocation(LayoutUnit(1.3), LayoutUnit(1.6)),
                        NGPhysicalSize(LayoutUnit(5.8), LayoutUnit(4.3)));
  snapped_rect = rect.SnapToDevicePixels();
  EXPECT_EQ(1, snapped_rect.left);
  EXPECT_EQ(2, snapped_rect.top);
  EXPECT_EQ(6, snapped_rect.width);
  EXPECT_EQ(4, snapped_rect.height);

  rect = NGPhysicalRect(NGPhysicalLocation(LayoutUnit(1.6), LayoutUnit(1.3)),
                        NGPhysicalSize(LayoutUnit(5.8), LayoutUnit(4.3)));
  snapped_rect = rect.SnapToDevicePixels();
  EXPECT_EQ(2, snapped_rect.left);
  EXPECT_EQ(1, snapped_rect.top);
  EXPECT_EQ(5, snapped_rect.width);
  EXPECT_EQ(5, snapped_rect.height);
}

}  // namespace

}  // namespace blink
