// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/ng_constraint_space_builder.h"

#include "third_party/blink/renderer/core/layout/ng/ng_layout_test.h"

namespace blink {
namespace {

using NGConstraintSpaceBuilderTest = NGLayoutTest;

// Asserts that indefinite inline length becomes initial containing
// block width for horizontal-tb inside vertical document.
TEST(NGConstraintSpaceBuilderTest, AvailableSizeFromHorizontalICB) {
  NGPhysicalSize icb_size{NGSizeIndefinite, LayoutUnit(51)};

  NGConstraintSpaceBuilder horizontal_builder(WritingMode::kHorizontalTb,
                                              icb_size);
  NGLogicalSize fixed_size{LayoutUnit(100), LayoutUnit(200)};
  NGLogicalSize indefinite_size{NGSizeIndefinite, NGSizeIndefinite};

  horizontal_builder.SetAvailableSize(fixed_size);
  horizontal_builder.SetPercentageResolutionSize(fixed_size);

  NGConstraintSpaceBuilder vertical_builder(
      *horizontal_builder.ToConstraintSpace(WritingMode::kHorizontalTb));

  vertical_builder.SetAvailableSize(indefinite_size);
  vertical_builder.SetPercentageResolutionSize(indefinite_size);
  scoped_refptr<NGConstraintSpace> space =
      vertical_builder.ToConstraintSpace(WritingMode::kVerticalLr);

  EXPECT_EQ(space->AvailableSize().inline_size, icb_size.height);
  EXPECT_EQ(space->PercentageResolutionSize().inline_size, icb_size.height);
};

// Asserts that indefinite inline length becomes initial containing
// block height for vertical-lr inside horizontal document.
TEST(NGConstraintSpaceBuilderTest, AvailableSizeFromVerticalICB) {
  NGPhysicalSize icb_size{LayoutUnit(51), NGSizeIndefinite};

  NGConstraintSpaceBuilder horizontal_builder(WritingMode::kVerticalLr,
                                              icb_size);
  NGLogicalSize fixed_size{LayoutUnit(100), LayoutUnit(200)};
  NGLogicalSize indefinite_size{NGSizeIndefinite, NGSizeIndefinite};

  horizontal_builder.SetAvailableSize(fixed_size);
  horizontal_builder.SetPercentageResolutionSize(fixed_size);

  NGConstraintSpaceBuilder vertical_builder(
      *horizontal_builder.ToConstraintSpace(WritingMode::kVerticalLr));

  vertical_builder.SetAvailableSize(indefinite_size);
  vertical_builder.SetPercentageResolutionSize(indefinite_size);
  scoped_refptr<NGConstraintSpace> space =
      vertical_builder.ToConstraintSpace(WritingMode::kHorizontalTb);

  EXPECT_EQ(space->AvailableSize().inline_size, icb_size.width);
  EXPECT_EQ(space->PercentageResolutionSize().inline_size, icb_size.width);
};

}  // namespace

}  // namespace blink
