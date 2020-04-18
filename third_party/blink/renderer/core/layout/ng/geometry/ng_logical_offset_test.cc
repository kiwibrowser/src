// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/geometry/ng_logical_offset.h"
#include "third_party/blink/renderer/core/layout/ng/geometry/ng_physical_offset.h"
#include "third_party/blink/renderer/core/layout/ng/geometry/ng_physical_size.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

namespace {

TEST(NGGeometryUnitsTest, ConvertLogicalOffsetToPhysicalOffset) {
  NGLogicalOffset logical_offset(LayoutUnit(20), LayoutUnit(30));
  NGPhysicalSize outer_size(LayoutUnit(300), LayoutUnit(400));
  NGPhysicalSize inner_size(LayoutUnit(5), LayoutUnit(65));
  NGPhysicalOffset offset;

  offset = logical_offset.ConvertToPhysical(
      WritingMode::kHorizontalTb, TextDirection::kLtr, outer_size, inner_size);
  EXPECT_EQ(LayoutUnit(20), offset.left);
  EXPECT_EQ(LayoutUnit(30), offset.top);

  offset = logical_offset.ConvertToPhysical(
      WritingMode::kHorizontalTb, TextDirection::kRtl, outer_size, inner_size);
  EXPECT_EQ(LayoutUnit(275), offset.left);
  EXPECT_EQ(LayoutUnit(30), offset.top);

  offset = logical_offset.ConvertToPhysical(
      WritingMode::kVerticalRl, TextDirection::kLtr, outer_size, inner_size);
  EXPECT_EQ(LayoutUnit(265), offset.left);
  EXPECT_EQ(LayoutUnit(20), offset.top);

  offset = logical_offset.ConvertToPhysical(
      WritingMode::kVerticalRl, TextDirection::kRtl, outer_size, inner_size);
  EXPECT_EQ(LayoutUnit(265), offset.left);
  EXPECT_EQ(LayoutUnit(315), offset.top);

  offset = logical_offset.ConvertToPhysical(
      WritingMode::kSidewaysRl, TextDirection::kLtr, outer_size, inner_size);
  EXPECT_EQ(LayoutUnit(265), offset.left);
  EXPECT_EQ(LayoutUnit(20), offset.top);

  offset = logical_offset.ConvertToPhysical(
      WritingMode::kSidewaysRl, TextDirection::kRtl, outer_size, inner_size);
  EXPECT_EQ(LayoutUnit(265), offset.left);
  EXPECT_EQ(LayoutUnit(315), offset.top);

  offset = logical_offset.ConvertToPhysical(
      WritingMode::kVerticalLr, TextDirection::kLtr, outer_size, inner_size);
  EXPECT_EQ(LayoutUnit(30), offset.left);
  EXPECT_EQ(LayoutUnit(20), offset.top);

  offset = logical_offset.ConvertToPhysical(
      WritingMode::kVerticalLr, TextDirection::kRtl, outer_size, inner_size);
  EXPECT_EQ(LayoutUnit(30), offset.left);
  EXPECT_EQ(LayoutUnit(315), offset.top);

  offset = logical_offset.ConvertToPhysical(
      WritingMode::kSidewaysLr, TextDirection::kLtr, outer_size, inner_size);
  EXPECT_EQ(LayoutUnit(30), offset.left);
  EXPECT_EQ(LayoutUnit(315), offset.top);

  offset = logical_offset.ConvertToPhysical(
      WritingMode::kSidewaysLr, TextDirection::kRtl, outer_size, inner_size);
  EXPECT_EQ(LayoutUnit(30), offset.left);
  EXPECT_EQ(LayoutUnit(20), offset.top);
}

}  // namespace

}  // namespace blink
