// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/geometry/ng_physical_offset.h"
#include "third_party/blink/renderer/core/layout/ng/geometry/ng_logical_offset.h"
#include "third_party/blink/renderer/core/layout/ng/geometry/ng_physical_size.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

namespace {

TEST(NGGeometryUnitsTest, ConvertPhysicalOffsetToLogicalOffset) {
  NGPhysicalOffset physical_offset(LayoutUnit(20), LayoutUnit(30));
  NGPhysicalSize outer_size(LayoutUnit(300), LayoutUnit(400));
  NGPhysicalSize inner_size(LayoutUnit(5), LayoutUnit(65));
  NGLogicalOffset offset;

  offset = physical_offset.ConvertToLogical(
      WritingMode::kHorizontalTb, TextDirection::kLtr, outer_size, inner_size);
  EXPECT_EQ(LayoutUnit(20), offset.inline_offset);
  EXPECT_EQ(LayoutUnit(30), offset.block_offset);

  offset = physical_offset.ConvertToLogical(
      WritingMode::kHorizontalTb, TextDirection::kRtl, outer_size, inner_size);
  EXPECT_EQ(LayoutUnit(275), offset.inline_offset);
  EXPECT_EQ(LayoutUnit(30), offset.block_offset);

  offset = physical_offset.ConvertToLogical(
      WritingMode::kVerticalRl, TextDirection::kLtr, outer_size, inner_size);
  EXPECT_EQ(LayoutUnit(30), offset.inline_offset);
  EXPECT_EQ(LayoutUnit(275), offset.block_offset);

  offset = physical_offset.ConvertToLogical(
      WritingMode::kVerticalRl, TextDirection::kRtl, outer_size, inner_size);
  EXPECT_EQ(LayoutUnit(305), offset.inline_offset);
  EXPECT_EQ(LayoutUnit(275), offset.block_offset);

  offset = physical_offset.ConvertToLogical(
      WritingMode::kSidewaysRl, TextDirection::kLtr, outer_size, inner_size);
  EXPECT_EQ(LayoutUnit(30), offset.inline_offset);
  EXPECT_EQ(LayoutUnit(275), offset.block_offset);

  offset = physical_offset.ConvertToLogical(
      WritingMode::kSidewaysRl, TextDirection::kRtl, outer_size, inner_size);
  EXPECT_EQ(LayoutUnit(305), offset.inline_offset);
  EXPECT_EQ(LayoutUnit(275), offset.block_offset);

  offset = physical_offset.ConvertToLogical(
      WritingMode::kVerticalLr, TextDirection::kLtr, outer_size, inner_size);
  EXPECT_EQ(LayoutUnit(30), offset.inline_offset);
  EXPECT_EQ(LayoutUnit(20), offset.block_offset);

  offset = physical_offset.ConvertToLogical(
      WritingMode::kVerticalLr, TextDirection::kRtl, outer_size, inner_size);
  EXPECT_EQ(LayoutUnit(305), offset.inline_offset);
  EXPECT_EQ(LayoutUnit(20), offset.block_offset);

  offset = physical_offset.ConvertToLogical(
      WritingMode::kSidewaysLr, TextDirection::kLtr, outer_size, inner_size);
  EXPECT_EQ(LayoutUnit(305), offset.inline_offset);
  EXPECT_EQ(LayoutUnit(20), offset.block_offset);

  offset = physical_offset.ConvertToLogical(
      WritingMode::kSidewaysLr, TextDirection::kRtl, outer_size, inner_size);
  EXPECT_EQ(LayoutUnit(30), offset.inline_offset);
  EXPECT_EQ(LayoutUnit(20), offset.block_offset);
}

}  // namespace

}  // namespace blink
