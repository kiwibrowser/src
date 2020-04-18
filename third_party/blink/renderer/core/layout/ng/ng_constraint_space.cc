// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/ng_constraint_space.h"

#include <algorithm>
#include <memory>

#include "third_party/blink/renderer/core/layout/layout_block.h"
#include "third_party/blink/renderer/core/layout/layout_flexible_box.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/layout/ng/ng_constraint_space_builder.h"
#include "third_party/blink/renderer/core/layout/ng/ng_length_utils.h"

namespace blink {

NGConstraintSpace::NGConstraintSpace(
    WritingMode writing_mode,
    bool is_orthogonal_writing_mode_root,
    TextDirection direction,
    NGLogicalSize available_size,
    NGLogicalSize percentage_resolution_size,
    LayoutUnit parent_percentage_resolution_inline_size,
    NGPhysicalSize initial_containing_block_size,
    LayoutUnit fragmentainer_block_size,
    LayoutUnit fragmentainer_space_at_bfc_start,
    bool is_fixed_size_inline,
    bool is_fixed_size_block,
    bool fixed_size_block_is_definite,
    bool is_shrink_to_fit,
    bool is_intermediate_layout,
    NGFragmentationType block_direction_fragmentation_type,
    bool separate_leading_fragmentainer_margins,
    bool is_new_fc,
    bool is_anonymous,
    bool use_first_line_style,
    bool should_force_clearance,
    NGFloatTypes adjoining_floats,
    const NGMarginStrut& margin_strut,
    const NGBfcOffset& bfc_offset,
    const base::Optional<NGBfcOffset>& floats_bfc_offset,
    const NGExclusionSpace& exclusion_space,
    LayoutUnit clearance_offset,
    Vector<NGBaselineRequest>& baseline_requests)
    : available_size_(available_size),
      percentage_resolution_size_(percentage_resolution_size),
      parent_percentage_resolution_inline_size_(
          parent_percentage_resolution_inline_size),
      initial_containing_block_size_(initial_containing_block_size),
      fragmentainer_block_size_(fragmentainer_block_size),
      fragmentainer_space_at_bfc_start_(fragmentainer_space_at_bfc_start),
      is_fixed_size_inline_(is_fixed_size_inline),
      is_fixed_size_block_(is_fixed_size_block),
      fixed_size_block_is_definite_(fixed_size_block_is_definite),
      is_shrink_to_fit_(is_shrink_to_fit),
      is_intermediate_layout_(is_intermediate_layout),
      block_direction_fragmentation_type_(block_direction_fragmentation_type),
      separate_leading_fragmentainer_margins_(
          separate_leading_fragmentainer_margins),
      is_new_fc_(is_new_fc),
      is_anonymous_(is_anonymous),
      use_first_line_style_(use_first_line_style),
      should_force_clearance_(should_force_clearance),
      adjoining_floats_(adjoining_floats),
      writing_mode_(static_cast<unsigned>(writing_mode)),
      is_orthogonal_writing_mode_root_(is_orthogonal_writing_mode_root),
      direction_(static_cast<unsigned>(direction)),
      margin_strut_(margin_strut),
      bfc_offset_(bfc_offset),
      floats_bfc_offset_(floats_bfc_offset),
      exclusion_space_(std::make_unique<NGExclusionSpace>(exclusion_space)),
      clearance_offset_(clearance_offset) {
  baseline_requests_.swap(baseline_requests);
}

scoped_refptr<NGConstraintSpace> NGConstraintSpace::CreateFromLayoutObject(
    const LayoutBox& box) {
  auto writing_mode = box.StyleRef().GetWritingMode();
  bool parallel_containing_block = IsParallelWritingMode(
      box.ContainingBlock()->StyleRef().GetWritingMode(), writing_mode);
  bool fixed_inline = false, fixed_block = false;
  bool fixed_block_is_definite = true;

  LayoutUnit available_logical_width;
  if (parallel_containing_block &&
      box.HasOverrideContainingBlockContentLogicalWidth()) {
    // Grid layout sets OverrideContainingBlockContentLogicalWidth|Height
    available_logical_width = box.OverrideContainingBlockContentLogicalWidth();
  } else if (!parallel_containing_block &&
             box.HasOverrideContainingBlockContentLogicalHeight()) {
    available_logical_width = box.OverrideContainingBlockContentLogicalHeight();
  } else {
    if (parallel_containing_block)
      available_logical_width = box.ContainingBlockLogicalWidthForContent();
    else
      available_logical_width = box.PerpendicularContainingBlockLogicalHeight();
  }
  available_logical_width = std::max(LayoutUnit(), available_logical_width);

  LayoutUnit available_logical_height;
  if (parallel_containing_block &&
      box.HasOverrideContainingBlockContentLogicalHeight()) {
    // Grid layout sets OverrideContainingBlockContentLogicalWidth|Height
    available_logical_height =
        box.OverrideContainingBlockContentLogicalHeight();
  } else if (!parallel_containing_block &&
             box.HasOverrideContainingBlockContentLogicalWidth()) {
    available_logical_height = box.OverrideContainingBlockContentLogicalWidth();
  } else {
    if (!box.Parent()) {
      available_logical_height = box.View()->ViewLogicalHeightForPercentages();
    } else if (box.ContainingBlock()) {
      if (parallel_containing_block) {
        available_logical_height =
            box.ContainingBlock()
                ->AvailableLogicalHeightForPercentageComputation();
      } else {
        available_logical_height = box.ContainingBlockLogicalWidthForContent();
      }
    }
  }
  NGLogicalSize percentage_size = {available_logical_width,
                                   available_logical_height};
  NGLogicalSize available_size = percentage_size;
  if (box.HasOverrideLogicalWidth()) {
    available_size.inline_size = box.OverrideLogicalWidth();
    fixed_inline = true;
  }
  if (box.HasOverrideLogicalHeight()) {
    available_size.block_size = box.OverrideLogicalHeight();
    fixed_block = true;
  }
  if (box.IsFlexItem() && fixed_block) {
    fixed_block_is_definite =
        ToLayoutFlexibleBox(box.Parent())
            ->UseOverrideLogicalHeightForPerentageResolution(box);
  }

  bool is_new_fc = true;
  // TODO(ikilpatrick): This DCHECK needs to be enabled once we've switched
  // LayoutTableCell, etc over to LayoutNG.
  //
  // We currently need to "force" LayoutNG roots to be formatting contexts so
  // that floats have layout performed on them.
  //
  // DCHECK(is_new_fc,
  //  box.IsLayoutBlock() && ToLayoutBlock(box).CreatesNewFormattingContext());

  IntSize icb_size = box.View()->GetLayoutSize(kExcludeScrollbars);
  NGPhysicalSize initial_containing_block_size{LayoutUnit(icb_size.Width()),
                                               LayoutUnit(icb_size.Height())};

  // ICB cannot be indefinite by the spec.
  DCHECK_GE(initial_containing_block_size.width, LayoutUnit());
  DCHECK_GE(initial_containing_block_size.height, LayoutUnit());

  NGConstraintSpaceBuilder builder(writing_mode, initial_containing_block_size);

  if (!box.IsWritingModeRoot()) {
    // Add all types because we don't know which baselines will be requested.
    FontBaseline baseline_type = box.StyleRef().GetFontBaseline();
    bool synthesize_inline_block_baseline =
        box.IsLayoutBlock() &&
        ToLayoutBlock(box).UseLogicalBottomMarginEdgeForInlineBlockBaseline();
    if (!synthesize_inline_block_baseline) {
      builder.AddBaselineRequest(
          {NGBaselineAlgorithmType::kAtomicInline, baseline_type});
    }
    builder.AddBaselineRequest(
        {NGBaselineAlgorithmType::kFirstLine, baseline_type});
  }

  return builder.SetAvailableSize(available_size)
      .SetPercentageResolutionSize(percentage_size)
      .SetIsFixedSizeInline(fixed_inline)
      .SetIsFixedSizeBlock(fixed_block)
      .SetFixedSizeBlockIsDefinite(fixed_block_is_definite)
      .SetIsShrinkToFit(
          box.SizesLogicalWidthToFitContent(box.StyleRef().LogicalWidth()))
      .SetIsNewFormattingContext(is_new_fc)
      .SetTextDirection(box.StyleRef().Direction())
      .ToConstraintSpace(writing_mode);
}

LayoutUnit
NGConstraintSpace::PercentageResolutionInlineSizeForParentWritingMode() const {
  if (!IsOrthogonalWritingModeRoot())
    return PercentageResolutionSize().inline_size;
  if (PercentageResolutionSize().block_size != NGSizeIndefinite)
    return PercentageResolutionSize().block_size;
  if (IsHorizontalWritingMode(GetWritingMode()))
    return InitialContainingBlockSize().height;
  return InitialContainingBlockSize().width;
}

LayoutUnit NGConstraintSpace::ParentPercentageResolutionInlineSize() const {
  if (parent_percentage_resolution_inline_size_ != NGSizeIndefinite)
    return parent_percentage_resolution_inline_size_;
  return initial_containing_block_size_.ConvertToLogical(GetWritingMode())
      .inline_size;
}

NGFragmentationType NGConstraintSpace::BlockFragmentationType() const {
  return static_cast<NGFragmentationType>(block_direction_fragmentation_type_);
}

bool NGConstraintSpace::operator==(const NGConstraintSpace& other) const {
  if (exclusion_space_ && other.exclusion_space_ &&
      *exclusion_space_ != *other.exclusion_space_)
    return false;

  return available_size_ == other.available_size_ &&
         percentage_resolution_size_ == other.percentage_resolution_size_ &&
         parent_percentage_resolution_inline_size_ ==
             other.parent_percentage_resolution_inline_size_ &&
         initial_containing_block_size_ ==
             other.initial_containing_block_size_ &&
         fragmentainer_block_size_ == other.fragmentainer_block_size_ &&
         fragmentainer_space_at_bfc_start_ ==
             other.fragmentainer_space_at_bfc_start_ &&
         is_fixed_size_inline_ == other.is_fixed_size_inline_ &&
         is_fixed_size_block_ == other.is_fixed_size_block_ &&
         is_shrink_to_fit_ == other.is_shrink_to_fit_ &&
         is_intermediate_layout_ == other.is_intermediate_layout_ &&
         block_direction_fragmentation_type_ ==
             other.block_direction_fragmentation_type_ &&
         is_new_fc_ == other.is_new_fc_ &&
         separate_leading_fragmentainer_margins_ ==
             other.separate_leading_fragmentainer_margins_ &&
         is_anonymous_ == other.is_anonymous_ &&
         should_force_clearance_ == other.should_force_clearance_ &&
         adjoining_floats_ == other.adjoining_floats_ &&
         writing_mode_ == other.writing_mode_ &&
         direction_ == other.direction_ &&
         margin_strut_ == other.margin_strut_ &&
         bfc_offset_ == other.bfc_offset_ &&
         floats_bfc_offset_ == other.floats_bfc_offset_ &&
         clearance_offset_ == other.clearance_offset_ &&
         baseline_requests_ == other.baseline_requests_;
}

bool NGConstraintSpace::operator!=(const NGConstraintSpace& other) const {
  return !(*this == other);
}

String NGConstraintSpace::ToString() const {
  return String::Format("Offset: %s,%s Size: %sx%s Clearance: %s",
                        bfc_offset_.line_offset.ToString().Ascii().data(),
                        bfc_offset_.block_offset.ToString().Ascii().data(),
                        AvailableSize().inline_size.ToString().Ascii().data(),
                        AvailableSize().block_size.ToString().Ascii().data(),
                        HasClearanceOffset()
                            ? ClearanceOffset().ToString().Ascii().data()
                            : "none");
}

}  // namespace blink
