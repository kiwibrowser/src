// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/ng_length_utils.h"

#include <algorithm>
#include "base/optional.h"
#include "third_party/blink/renderer/core/layout/layout_box.h"
#include "third_party/blink/renderer/core/layout/layout_table_cell.h"
#include "third_party/blink/renderer/core/layout/ng/ng_block_node.h"
#include "third_party/blink/renderer/core/layout/ng/ng_constraint_space.h"
#include "third_party/blink/renderer/core/layout/ng/ng_constraint_space_builder.h"
#include "third_party/blink/renderer/core/style/computed_style.h"
#include "third_party/blink/renderer/platform/layout_unit.h"
#include "third_party/blink/renderer/platform/length.h"

namespace blink {

bool NeedMinMaxSize(const NGConstraintSpace& constraint_space,
                    const ComputedStyle& style) {
  // This check is technically too broad (fill-available does not need intrinsic
  // size computation) but that's a rare case and only affects performance, not
  // correctness.
  return constraint_space.IsShrinkToFit() || NeedMinMaxSize(style);
}

bool NeedMinMaxSize(const ComputedStyle& style) {
  return style.LogicalWidth().IsIntrinsic() ||
         style.LogicalMinWidth().IsIntrinsic() ||
         style.LogicalMaxWidth().IsIntrinsic();
}

bool NeedMinMaxSizeForContentContribution(WritingMode mode,
                                          const ComputedStyle& style) {
  // During the intrinsic sizes pass percentages/calc() are defined to behave
  // like 'auto'. As a result we need to calculate the intrinsic sizes for any
  // children with percentages. E.g.
  // <div style="float:left;">
  //   <div style="width:30%;">text text</div>
  // </div>
  if (mode == WritingMode::kHorizontalTb) {
    return style.Width().IsIntrinsicOrAuto() ||
           style.Width().IsPercentOrCalc() || style.MinWidth().IsIntrinsic() ||
           style.MaxWidth().IsIntrinsic();
  }
  return style.Height().IsIntrinsicOrAuto() ||
         style.Height().IsPercentOrCalc() || style.MinHeight().IsIntrinsic() ||
         style.MaxHeight().IsIntrinsic();
}

LayoutUnit ResolveInlineLength(const NGConstraintSpace& constraint_space,
                               const ComputedStyle& style,
                               const base::Optional<MinMaxSize>& min_and_max,
                               const Length& length,
                               LengthResolveType type,
                               LengthResolvePhase phase) {
  DCHECK_GE(constraint_space.AvailableSize().inline_size, LayoutUnit());
  DCHECK_GE(constraint_space.PercentageResolutionSize().inline_size,
            LayoutUnit());
  DCHECK_EQ(constraint_space.GetWritingMode(), style.GetWritingMode());

  if (constraint_space.IsAnonymous())
    return constraint_space.AvailableSize().inline_size;

  if (length.IsMaxSizeNone()) {
    DCHECK_EQ(type, LengthResolveType::kMaxSize);
    return LayoutUnit::Max();
  }

  NGBoxStrut border_and_padding = ComputeBorders(constraint_space, style) +
                                  ComputePadding(constraint_space, style);

  if (type == LengthResolveType::kMinSize && length.IsAuto())
    return border_and_padding.InlineSum();

  // Check if we shouldn't resolve a percentage/calc() if we are in the
  // intrinsic sizes phase.
  if (phase == LengthResolvePhase::kIntrinsic && length.IsPercentOrCalc()) {
    // min-width/min-height should be "0", i.e. no min limit is applied.
    if (type == LengthResolveType::kMinSize)
      return border_and_padding.InlineSum();

    // max-width/max-height becomes "infinity", i.e. no max limit is applied.
    if (type == LengthResolveType::kMaxSize)
      return LayoutUnit::Max();
  }

  switch (length.GetType()) {
    case kAuto:
    case kFillAvailable: {
      LayoutUnit content_size = constraint_space.AvailableSize().inline_size;
      NGBoxStrut margins = ComputeMarginsForSelf(constraint_space, style);
      return std::max(border_and_padding.InlineSum(),
                      content_size - margins.InlineSum());
    }
    case kPercent:
    case kFixed:
    case kCalculated: {
      LayoutUnit percentage_resolution_size =
          constraint_space.PercentageResolutionSize().inline_size;
      LayoutUnit value = ValueForLength(length, percentage_resolution_size);
      if (style.BoxSizing() == EBoxSizing::kContentBox) {
        value += border_and_padding.InlineSum();
      } else {
        value = std::max(border_and_padding.InlineSum(), value);
      }
      return value;
    }
    case kMinContent:
    case kMaxContent:
    case kFitContent: {
      DCHECK(min_and_max.has_value());
      LayoutUnit available_size = constraint_space.AvailableSize().inline_size;
      LayoutUnit value;
      if (length.IsMinContent()) {
        value = min_and_max->min_size;
      } else if (length.IsMaxContent() || available_size == LayoutUnit::Max()) {
        // If the available space is infinite, fit-content resolves to
        // max-content. See css-sizing section 2.1.
        value = min_and_max->max_size;
      } else {
        NGBoxStrut margins = ComputeMarginsForSelf(constraint_space, style);
        LayoutUnit fill_available =
            std::max(LayoutUnit(), available_size - margins.InlineSum());
        value = min_and_max->ShrinkToFit(fill_available);
      }
      return value;
    }
    case kDeviceWidth:
    case kDeviceHeight:
    case kExtendToZoom:
      NOTREACHED() << "These should only be used for viewport definitions";
      FALLTHROUGH;
    case kMaxSizeNone:
    default:
      NOTREACHED();
      return border_and_padding.InlineSum();
  }
}

LayoutUnit ResolveBlockLength(const NGConstraintSpace& constraint_space,
                              const ComputedStyle& style,
                              const Length& length,
                              LayoutUnit content_size,
                              LengthResolveType type,
                              LengthResolvePhase phase) {
  DCHECK_EQ(constraint_space.GetWritingMode(), style.GetWritingMode());

  if (constraint_space.IsAnonymous())
    return content_size;

  if (length.IsMaxSizeNone()) {
    DCHECK_EQ(type, LengthResolveType::kMaxSize);
    return LayoutUnit::Max();
  }

  NGBoxStrut border_and_padding = ComputeBorders(constraint_space, style) +
                                  ComputePadding(constraint_space, style);

  if (type == LengthResolveType::kMinSize && length.IsAuto())
    return border_and_padding.BlockSum();

  bool is_percentage_indefinite =
      constraint_space.PercentageResolutionSize().block_size ==
      NGSizeIndefinite;

  // Check if we can't/shouldn't resolve a percentage/calc() - because the
  // percentage resolution size is indefinite or because we are in the
  // intrinsic sizes phase.
  if ((phase == LengthResolvePhase::kIntrinsic || is_percentage_indefinite) &&
      length.IsPercentOrCalc()) {
    // min-width/min-height should be "0", i.e. no min limit is applied.
    if (type == LengthResolveType::kMinSize)
      return border_and_padding.BlockSum();

    // max-width/max-height becomes "infinity", i.e. no max limit is applied.
    if (type == LengthResolveType::kMaxSize)
      return LayoutUnit::Max();

    // width/height becomes "auto", so we can just return the content size.
    DCHECK_EQ(type, LengthResolveType::kContentSize);
    return content_size;
  }

  switch (length.GetType()) {
    case kFillAvailable: {
      LayoutUnit content_size = constraint_space.AvailableSize().block_size;
      NGBoxStrut margins = ComputeMarginsForSelf(constraint_space, style);
      return std::max(border_and_padding.BlockSum(),
                      content_size - margins.BlockSum());
    }
    case kPercent:
    case kFixed:
    case kCalculated: {
      LayoutUnit percentage_resolution_size =
          constraint_space.PercentageResolutionSize().block_size;
      LayoutUnit value = ValueForLength(length, percentage_resolution_size);
      if (style.BoxSizing() == EBoxSizing::kContentBox) {
        value += border_and_padding.BlockSum();
      } else {
        value = std::max(border_and_padding.BlockSum(), value);
      }
      return value;
    }
    case kAuto:
    case kMinContent:
    case kMaxContent:
    case kFitContent:
#if DCHECK_IS_ON()
      // Due to how content_size is calculated, it should always include border
      // and padding. We cannot check for this if we are block-fragmented,
      // though, because then the block-start border/padding may be in a
      // different fragmentainer than the block-end border/padding.
      if (content_size != LayoutUnit(-1) &&
          !constraint_space.HasBlockFragmentation())
        DCHECK_GE(content_size, border_and_padding.BlockSum());
#endif  // DCHECK_IS_ON()
      return content_size;
    case kDeviceWidth:
    case kDeviceHeight:
    case kExtendToZoom:
      NOTREACHED() << "These should only be used for viewport definitions";
      FALLTHROUGH;
    case kMaxSizeNone:
    default:
      NOTREACHED();
      return border_and_padding.BlockSum();
  }
}

LayoutUnit ResolveMarginPaddingLength(const NGConstraintSpace& constraint_space,
                                      const Length& length) {
  DCHECK_GE(constraint_space.AvailableSize().inline_size, LayoutUnit());

  // Margins and padding always get computed relative to the inline size:
  // https://www.w3.org/TR/CSS2/box.html#value-def-margin-width
  // https://www.w3.org/TR/CSS2/box.html#value-def-padding-width
  switch (length.GetType()) {
    case kAuto:
      return LayoutUnit();
    case kPercent:
    case kFixed:
    case kCalculated: {
      LayoutUnit percentage_resolution_size =
          constraint_space.PercentageResolutionInlineSizeForParentWritingMode();
      return ValueForLength(length, percentage_resolution_size);
    }
    case kMinContent:
    case kMaxContent:
    case kFillAvailable:
    case kFitContent:
    case kExtendToZoom:
    case kDeviceWidth:
    case kDeviceHeight:
    case kMaxSizeNone:
      FALLTHROUGH;
    default:
      NOTREACHED();
      return LayoutUnit();
  }
}

MinMaxSize ComputeMinAndMaxContentContribution(
    WritingMode writing_mode,
    const ComputedStyle& style,
    const base::Optional<MinMaxSize>& min_and_max) {
  // Synthesize a zero-sized constraint space for passing to
  // ResolveInlineLength.
  // The constraint space's writing mode has to match the style, so we can't
  // use the passed-in mode here.
  NGConstraintSpaceBuilder builder(
      style.GetWritingMode(),
      /* icb_size */ {NGSizeIndefinite, NGSizeIndefinite});
  scoped_refptr<NGConstraintSpace> space =
      builder.ToConstraintSpace(style.GetWritingMode());

  LayoutUnit content_size =
      min_and_max ? min_and_max->max_size : NGSizeIndefinite;

  MinMaxSize computed_sizes;
  Length inline_size = writing_mode == WritingMode::kHorizontalTb
                           ? style.Width()
                           : style.Height();
  if (inline_size.IsAuto() || inline_size.IsPercentOrCalc()) {
    CHECK(min_and_max.has_value());
    computed_sizes = *min_and_max;
  } else {
    if (IsParallelWritingMode(writing_mode, style.GetWritingMode())) {
      computed_sizes.min_size = computed_sizes.max_size = ResolveInlineLength(
          *space, style, min_and_max, inline_size,
          LengthResolveType::kContentSize, LengthResolvePhase::kIntrinsic);
    } else {
      computed_sizes.min_size = computed_sizes.max_size = ResolveBlockLength(
          *space, style, inline_size, content_size,
          LengthResolveType::kContentSize, LengthResolvePhase::kIntrinsic);
    }
  }

  Length max_length = writing_mode == WritingMode::kHorizontalTb
                          ? style.MaxWidth()
                          : style.MaxHeight();
  LayoutUnit max;
  if (IsParallelWritingMode(writing_mode, style.GetWritingMode())) {
    max = ResolveInlineLength(*space, style, min_and_max, max_length,
                              LengthResolveType::kMaxSize,
                              LengthResolvePhase::kIntrinsic);
  } else {
    max = ResolveBlockLength(*space, style, max_length, content_size,
                             LengthResolveType::kMaxSize,
                             LengthResolvePhase::kIntrinsic);
  }
  computed_sizes.min_size = std::min(computed_sizes.min_size, max);
  computed_sizes.max_size = std::min(computed_sizes.max_size, max);

  Length min_length = writing_mode == WritingMode::kHorizontalTb
                          ? style.MinWidth()
                          : style.MinHeight();
  LayoutUnit min;
  if (IsParallelWritingMode(writing_mode, style.GetWritingMode())) {
    min = ResolveInlineLength(*space, style, min_and_max, min_length,
                              LengthResolveType::kMinSize,
                              LengthResolvePhase::kIntrinsic);
  } else {
    min = ResolveBlockLength(*space, style, min_length, content_size,
                             LengthResolveType::kMinSize,
                             LengthResolvePhase::kIntrinsic);
  }
  computed_sizes.min_size = std::max(computed_sizes.min_size, min);
  computed_sizes.max_size = std::max(computed_sizes.max_size, min);

  return computed_sizes;
}

MinMaxSize ComputeMinAndMaxContentContribution(
    WritingMode writing_mode,
    NGLayoutInputNode node,
    const MinMaxSizeInput& input,
    const NGConstraintSpace* constraint_space) {
  base::Optional<MinMaxSize> minmax;
  if (NeedMinMaxSizeForContentContribution(writing_mode, node.Style())) {
    scoped_refptr<NGConstraintSpace> adjusted_constraint_space;
    if (constraint_space) {
      // TODO(layout-ng): Check if our constraint space produces spec-compliant
      // outputs.
      // It is important to set a floats bfc offset so that we don't get a
      // partial layout. It is also important that we shrink to fit, by
      // definition.
      NGConstraintSpaceBuilder builder(*constraint_space);
      builder.SetAvailableSize(constraint_space->AvailableSize())
          .SetFloatsBfcOffset(NGBfcOffset())
          .SetIsShrinkToFit(true);
      adjusted_constraint_space =
          builder.ToConstraintSpace(node.Style().GetWritingMode());
      constraint_space = adjusted_constraint_space.get();
    }
    minmax = node.ComputeMinMaxSize(writing_mode, input, constraint_space);
  }

  return ComputeMinAndMaxContentContribution(writing_mode, node.Style(),
                                             minmax);
}

LayoutUnit ComputeInlineSizeForFragment(
    const NGConstraintSpace& space,
    const ComputedStyle& style,
    const base::Optional<MinMaxSize>& min_and_max) {
  if (space.IsFixedSizeInline())
    return space.AvailableSize().inline_size;

  Length logical_width = style.LogicalWidth();
  if (logical_width.IsAuto() && space.IsShrinkToFit())
    logical_width = Length(kFitContent);

  LayoutUnit extent = ResolveInlineLength(
      space, style, min_and_max, logical_width, LengthResolveType::kContentSize,
      LengthResolvePhase::kLayout);

  LayoutUnit max = ResolveInlineLength(
      space, style, min_and_max, style.LogicalMaxWidth(),
      LengthResolveType::kMaxSize, LengthResolvePhase::kLayout);
  LayoutUnit min = ResolveInlineLength(
      space, style, min_and_max, style.LogicalMinWidth(),
      LengthResolveType::kMinSize, LengthResolvePhase::kLayout);
  return ConstrainByMinMax(extent, min, max);
}

LayoutUnit ComputeBlockSizeForFragment(
    const NGConstraintSpace& constraint_space,
    const ComputedStyle& style,
    LayoutUnit content_size) {
  if (constraint_space.IsFixedSizeBlock())
    return constraint_space.AvailableSize().block_size;

  if (style.Display() == EDisplay::kTableCell) {
    // All handled by the table layout code or not applicable.
    return content_size;
  }
  LayoutUnit extent = ResolveBlockLength(
      constraint_space, style, style.LogicalHeight(), content_size,
      LengthResolveType::kContentSize, LengthResolvePhase::kLayout);
  if (extent == NGSizeIndefinite) {
    DCHECK_EQ(content_size, NGSizeIndefinite);
    return extent;
  }

  LayoutUnit max = ResolveBlockLength(
      constraint_space, style, style.LogicalMaxHeight(), content_size,
      LengthResolveType::kMaxSize, LengthResolvePhase::kLayout);
  LayoutUnit min = ResolveBlockLength(
      constraint_space, style, style.LogicalMinHeight(), content_size,
      LengthResolveType::kMinSize, LengthResolvePhase::kLayout);

  return ConstrainByMinMax(extent, min, max);
}

// Computes size for a replaced element.
NGLogicalSize ComputeReplacedSize(
    const NGLayoutInputNode& node,
    const NGConstraintSpace& space,
    const base::Optional<MinMaxSize>& child_minmax) {
  DCHECK(node.IsReplaced());

  NGLogicalSize replaced_size;

  NGLogicalSize default_intrinsic_size;
  base::Optional<LayoutUnit> computed_inline_size;
  base::Optional<LayoutUnit> computed_block_size;
  NGLogicalSize aspect_ratio;

  node.IntrinsicSize(&default_intrinsic_size, &computed_inline_size,
                     &computed_block_size, &aspect_ratio);

  const ComputedStyle& style = node.Style();
  Length inline_length = style.LogicalWidth();
  Length block_length = style.LogicalHeight();

  // Compute inline size
  if (inline_length.IsAuto()) {
    if (block_length.IsAuto() || aspect_ratio.IsEmpty()) {
      // Use intrinsic values if inline_size cannot be computed from block_size.
      if (computed_inline_size.has_value())
        replaced_size.inline_size = computed_inline_size.value();
      else
        replaced_size.inline_size = default_intrinsic_size.inline_size;
      replaced_size.inline_size +=
          (ComputeBorders(space, style) + ComputePadding(space, style))
              .InlineSum();
    } else {
      // inline_size is computed from block_size.
      replaced_size.inline_size =
          ResolveBlockLength(
              space, style, block_length, default_intrinsic_size.block_size,
              LengthResolveType::kContentSize, LengthResolvePhase::kLayout) *
          aspect_ratio.inline_size / aspect_ratio.block_size;
    }
  } else {
    // inline_size is resolved directly.
    replaced_size.inline_size = ResolveInlineLength(
        space, style, child_minmax, inline_length,
        LengthResolveType::kContentSize, LengthResolvePhase::kLayout);
  }

  // Compute block size
  if (block_length.IsAuto()) {
    if (inline_length.IsAuto() || aspect_ratio.IsEmpty()) {
      // Use intrinsic values if block_size cannot be computed from inline_size.
      if (computed_block_size.has_value())
        replaced_size.block_size = LayoutUnit(computed_block_size.value());
      else
        replaced_size.block_size = default_intrinsic_size.block_size;
      replaced_size.block_size +=
          (ComputeBorders(space, style) + ComputePadding(space, style))
              .BlockSum();
    } else {
      // block_size is computed from inline_size.
      replaced_size.block_size =
          ResolveInlineLength(space, style, child_minmax, inline_length,
                              LengthResolveType::kContentSize,
                              LengthResolvePhase::kLayout) *
          aspect_ratio.block_size / aspect_ratio.inline_size;
    }
  } else {
    replaced_size.block_size = ResolveBlockLength(
        space, style, block_length, default_intrinsic_size.block_size,
        LengthResolveType::kContentSize, LengthResolvePhase::kLayout);
  }
  return replaced_size;
}

int ResolveUsedColumnCount(int computed_count,
                           LayoutUnit computed_size,
                           LayoutUnit used_gap,
                           LayoutUnit available_size) {
  if (computed_size == NGSizeIndefinite) {
    DCHECK(computed_count);
    return computed_count;
  }
  DCHECK_GT(computed_size, LayoutUnit());
  int count_from_width =
      ((available_size + used_gap) / (computed_size + used_gap)).ToInt();
  count_from_width = std::max(1, count_from_width);
  if (!computed_count)
    return count_from_width;
  return std::max(1, std::min(computed_count, count_from_width));
}

int ResolveUsedColumnCount(LayoutUnit available_size,
                           const ComputedStyle& style) {
  LayoutUnit computed_column_inline_size =
      style.HasAutoColumnWidth()
          ? NGSizeIndefinite
          : std::max(LayoutUnit(1), LayoutUnit(style.ColumnWidth()));
  LayoutUnit gap = ResolveUsedColumnGap(available_size, style);
  int computed_count = style.ColumnCount();
  return ResolveUsedColumnCount(computed_count, computed_column_inline_size,
                                gap, available_size);
}

LayoutUnit ResolveUsedColumnInlineSize(int computed_count,
                                       LayoutUnit computed_size,
                                       LayoutUnit used_gap,
                                       LayoutUnit available_size) {
  int used_count = ResolveUsedColumnCount(computed_count, computed_size,
                                          used_gap, available_size);
  return std::max(((available_size + used_gap) / used_count) - used_gap,
                  LayoutUnit());
}

LayoutUnit ResolveUsedColumnInlineSize(LayoutUnit available_size,
                                       const ComputedStyle& style) {
  // Should only attempt to resolve this if columns != auto.
  DCHECK(!style.HasAutoColumnCount() || !style.HasAutoColumnWidth());

  LayoutUnit computed_size =
      style.HasAutoColumnWidth()
          ? NGSizeIndefinite
          : std::max(LayoutUnit(1), LayoutUnit(style.ColumnWidth()));
  int computed_count = style.HasAutoColumnCount() ? 0 : style.ColumnCount();
  LayoutUnit used_gap = ResolveUsedColumnGap(available_size, style);
  return ResolveUsedColumnInlineSize(computed_count, computed_size, used_gap,
                                     available_size);
}

LayoutUnit ResolveUsedColumnGap(LayoutUnit available_size,
                                const ComputedStyle& style) {
  if (style.ColumnGap().IsNormal())
    return LayoutUnit(style.GetFontDescription().ComputedPixelSize());
  return ValueForLength(style.ColumnGap().GetLength(), available_size);
}

NGPhysicalBoxStrut ComputePhysicalMargins(
    const NGConstraintSpace& constraint_space,
    const ComputedStyle& style) {
  if (constraint_space.IsAnonymous())
    return NGPhysicalBoxStrut();
  NGPhysicalBoxStrut physical_dim;
  physical_dim.left =
      ResolveMarginPaddingLength(constraint_space, style.MarginLeft());
  physical_dim.right =
      ResolveMarginPaddingLength(constraint_space, style.MarginRight());
  physical_dim.top =
      ResolveMarginPaddingLength(constraint_space, style.MarginTop());
  physical_dim.bottom =
      ResolveMarginPaddingLength(constraint_space, style.MarginBottom());
  return physical_dim;
}

NGBoxStrut ComputeMarginsFor(const NGConstraintSpace& constraint_space,
                             const ComputedStyle& style,
                             const NGConstraintSpace& compute_for) {
  return ComputePhysicalMargins(constraint_space, style)
      .ConvertToLogical(compute_for.GetWritingMode(), compute_for.Direction());
}

NGBoxStrut ComputeMarginsForContainer(const NGConstraintSpace& constraint_space,
                                      const ComputedStyle& style) {
  return ComputePhysicalMargins(constraint_space, style)
      .ConvertToLogical(constraint_space.GetWritingMode(),
                        constraint_space.Direction());
}

NGBoxStrut ComputeMarginsForVisualContainer(
    const NGConstraintSpace& constraint_space,
    const ComputedStyle& style) {
  return ComputePhysicalMargins(constraint_space, style)
      .ConvertToLogical(constraint_space.GetWritingMode(), TextDirection::kLtr);
}

NGBoxStrut ComputeMarginsForSelf(const NGConstraintSpace& constraint_space,
                                 const ComputedStyle& style) {
  return ComputePhysicalMargins(constraint_space, style)
      .ConvertToLogical(style.GetWritingMode(), style.Direction());
}

NGBoxStrut ComputeMinMaxMargins(const ComputedStyle& parent_style,
                                NGLayoutInputNode child) {
  // An inline child just produces line-boxes which don't have any margins.
  if (child.IsInline())
    return NGBoxStrut();

  Length inline_start_margin_length =
      child.Style().MarginStartUsing(parent_style);
  Length inline_end_margin_length = child.Style().MarginEndUsing(parent_style);

  // TODO(ikilpatrick): We may want to re-visit calculated margins at some
  // point. Currently "margin-left: calc(10px + 50%)" will resolve to 0px, but
  // 10px would be more correct, (as percentages resolve to zero).
  NGBoxStrut margins;
  if (inline_start_margin_length.IsFixed())
    margins.inline_start = LayoutUnit(inline_start_margin_length.Value());
  if (inline_end_margin_length.IsFixed())
    margins.inline_end = LayoutUnit(inline_end_margin_length.Value());

  return margins;
}

NGBoxStrut ComputeBorders(const NGConstraintSpace& constraint_space,
                          const ComputedStyle& style) {
  // If we are producing an anonymous fragment (e.g. a column) we shouldn't
  // have any borders.
  if (constraint_space.IsAnonymous())
    return NGBoxStrut();

  NGBoxStrut borders;
  borders.inline_start = LayoutUnit(style.BorderStartWidth());
  borders.inline_end = LayoutUnit(style.BorderEndWidth());
  borders.block_start = LayoutUnit(style.BorderBeforeWidth());
  borders.block_end = LayoutUnit(style.BorderAfterWidth());
  return borders;
}

NGBoxStrut ComputePadding(const NGConstraintSpace& constraint_space,
                          const ComputedStyle& style) {
  // If we are producing an anonymous fragment (e.g. a column) we shouldn't
  // have any padding.
  if (constraint_space.IsAnonymous())
    return NGBoxStrut();

  NGBoxStrut padding;
  padding.inline_start =
      ResolveMarginPaddingLength(constraint_space, style.PaddingStart());
  padding.inline_end =
      ResolveMarginPaddingLength(constraint_space, style.PaddingEnd());
  padding.block_start =
      ResolveMarginPaddingLength(constraint_space, style.PaddingBefore());
  padding.block_end =
      ResolveMarginPaddingLength(constraint_space, style.PaddingAfter());
  return padding;
}

void ApplyAutoMargins(const ComputedStyle& style,
                      const ComputedStyle& containing_block_style,
                      LayoutUnit available_inline_size,
                      LayoutUnit inline_size,
                      NGBoxStrut* margins) {
  DCHECK(margins) << "Margins cannot be NULL here";
  const LayoutUnit used_space = inline_size + margins->InlineSum();
  const LayoutUnit available_space = available_inline_size - used_space;
  if (available_space > LayoutUnit()) {
    bool start_auto = style.MarginStartUsing(containing_block_style).IsAuto();
    bool end_auto = style.MarginEndUsing(containing_block_style).IsAuto();
    enum EBlockAlignment { kStart, kCenter, kEnd };
    EBlockAlignment alignment;
    if (start_auto || end_auto) {
      alignment = start_auto ? (end_auto ? kCenter : kEnd) : kStart;
    } else {
      // If none of the inline margins are auto, look for -webkit- text-align
      // values (which are really about block alignment). These are typically
      // mapped from the legacy "align" HTML attribute.
      switch (containing_block_style.GetTextAlign()) {
        case ETextAlign::kWebkitLeft:
          alignment =
              containing_block_style.IsLeftToRightDirection() ? kStart : kEnd;
          break;
        case ETextAlign::kWebkitRight:
          alignment =
              containing_block_style.IsLeftToRightDirection() ? kEnd : kStart;
          break;
        case ETextAlign::kWebkitCenter:
          alignment = kCenter;
          break;
        default:
          alignment = kStart;
          break;
      }
    }
    if (alignment == kCenter)
      margins->inline_start += available_space / 2;
    else if (alignment == kEnd)
      margins->inline_start += available_space;
  }
  margins->inline_end =
      available_inline_size - inline_size - margins->inline_start;
}

LayoutUnit LineOffsetForTextAlign(ETextAlign text_align,
                                  TextDirection direction,
                                  LayoutUnit space_left,
                                  LayoutUnit trailing_spaces_width) {
  bool is_ltr = IsLtr(direction);
  if (text_align == ETextAlign::kStart || text_align == ETextAlign::kJustify)
    text_align = is_ltr ? ETextAlign::kLeft : ETextAlign::kRight;
  else if (text_align == ETextAlign::kEnd)
    text_align = is_ltr ? ETextAlign::kRight : ETextAlign::kLeft;

  switch (text_align) {
    case ETextAlign::kLeft:
    case ETextAlign::kWebkitLeft: {
      // The direction of the block should determine what happens with wide
      // lines. In particular with RTL blocks, wide lines should still spill
      // out to the left.
      if (is_ltr)
        return LayoutUnit();
      return space_left.ClampPositiveToZero();
    }
    case ETextAlign::kRight:
    case ETextAlign::kWebkitRight: {
      // In RTL, trailing spaces appear on the left of the line.
      if (UNLIKELY(!is_ltr))
        return space_left - trailing_spaces_width;
      // Wide lines spill out of the block based off direction.
      // So even if text-align is right, if direction is LTR, wide lines
      // should overflow out of the right side of the block.
      if (space_left > LayoutUnit())
        return space_left;
      return LayoutUnit();
    }
    case ETextAlign::kCenter:
    case ETextAlign::kWebkitCenter: {
      if (is_ltr)
        return (space_left / 2).ClampNegativeToZero();
      // In RTL, trailing spaces appear on the left of the line.
      if (space_left > LayoutUnit())
        return (space_left / 2).ClampNegativeToZero() - trailing_spaces_width;
      // In RTL, wide lines should spill out to the left, same as kRight.
      return space_left - trailing_spaces_width;
    }
    default:
      NOTREACHED();
      return LayoutUnit();
  }
}

LayoutUnit ConstrainByMinMax(LayoutUnit length,
                             LayoutUnit min,
                             LayoutUnit max) {
  return std::max(min, std::min(length, max));
}

NGBoxStrut CalculateBorderScrollbarPadding(
    const NGConstraintSpace& constraint_space,
    const NGBlockNode node) {
  const ComputedStyle& style = node.Style();

  // If we are producing an anonymous fragment (e.g. a column), it has no
  // borders, padding or scrollbars. Using the ones from the container can only
  // cause trouble.
  if (constraint_space.IsAnonymous())
    return NGBoxStrut();
  NGBoxStrut border_intrinsic_padding;
  if (node.GetLayoutObject()->IsTableCell()) {
    // Use values calculated by the table layout code
    const LayoutTableCell* cell = ToLayoutTableCell(node.GetLayoutObject());
    // TODO(karlo): intrinsic padding can sometimes be negative; that
    // seems insane, but works in the old code; in NG it trips
    // DCHECKs.
    border_intrinsic_padding = NGBoxStrut(
        cell->BorderStart(), cell->BorderEnd(),
        cell->BorderBefore() + LayoutUnit(cell->IntrinsicPaddingBefore()),
        cell->BorderAfter() + LayoutUnit(cell->IntrinsicPaddingAfter()));
  } else {
    border_intrinsic_padding = ComputeBorders(constraint_space, style);
  }
  return border_intrinsic_padding + ComputePadding(constraint_space, style) +
         node.GetScrollbarSizes();
}

NGLogicalSize CalculateContentBoxSize(
    const NGLogicalSize border_box_size,
    const NGBoxStrut& border_scrollbar_padding) {
  NGLogicalSize size = border_box_size;
  size.inline_size -= border_scrollbar_padding.InlineSum();
  size.inline_size = std::max(size.inline_size, LayoutUnit());

  // Our calculated block-axis size may still be indefinite. If so, just leave
  // the size as NGSizeIndefinite instead of subtracting borders and padding.
  if (size.block_size != NGSizeIndefinite) {
    size.block_size -= border_scrollbar_padding.BlockSum();
    size.block_size = std::max(size.block_size, LayoutUnit());
  }

  return size;
}

}  // namespace blink
