// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/ng_flex_layout_algorithm.h"

#include <memory>
#include "third_party/blink/renderer/core/layout/flexible_box_algorithm.h"
#include "third_party/blink/renderer/core/layout/layout_box.h"
#include "third_party/blink/renderer/core/layout/ng/ng_block_break_token.h"
#include "third_party/blink/renderer/core/layout/ng/ng_constraint_space.h"
#include "third_party/blink/renderer/core/layout/ng/ng_constraint_space_builder.h"
#include "third_party/blink/renderer/core/layout/ng/ng_length_utils.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

NGFlexLayoutAlgorithm::NGFlexLayoutAlgorithm(NGBlockNode node,
                                             const NGConstraintSpace& space,
                                             NGBreakToken* break_token)
    : NGLayoutAlgorithm(node, space, ToNGBlockBreakToken(break_token)) {}

scoped_refptr<NGLayoutResult> NGFlexLayoutAlgorithm::Layout() {
  DCHECK(!Style().IsColumnFlexDirection())
      << "Column flexboxes aren't supported yet";
  DCHECK(!NeedMinMaxSize(ConstraintSpace(), Style()))
      << "Don't support that yet";

  LayoutUnit container_logical_width = ComputeInlineSizeForFragment(
      ConstraintSpace(), Style(), /* MinMaxSize */ base::nullopt);

  Vector<FlexItem> flex_items;
  for (NGLayoutInputNode child = Node().FirstChild(); child;
       child = child.NextSibling()) {
    if (child.IsOutOfFlowPositioned())
      continue;
    // Assume row flexbox with no orthogonal items, which lets us just use
    // MinMaxSize for flex base size. An orthogonal item would need full layout.
    // TODO(layout-ng): Now that ComputeMinMaxSize takes a writing mode, this
    // should be easy to fix by just passing an appropriate constraint space to
    // ComputeMinMaxSize.
    DCHECK(IsParallelWritingMode(Node().Style().GetWritingMode(),
                                 child.Style().GetWritingMode()))
        << "Orthogonal items aren't supported yet.";
    MinMaxSizeInput zero_input;
    MinMaxSize min_max_sizes =
        child.ComputeMinMaxSize(ConstraintSpace().GetWritingMode(), zero_input);

    LayoutUnit flex_base_content_size;
    if (child.Style().FlexBasis().IsAuto() && child.Style().Width().IsAuto()) {
      flex_base_content_size = min_max_sizes.max_size;
    } else {
      Length length_to_resolve = child.Style().FlexBasis();
      if (length_to_resolve.IsAuto())
        length_to_resolve = child.Style().Width();
      DCHECK(!length_to_resolve.IsAuto());

      // TODO(dgrogan): ResolveInlineLength will handle all the types?
      DCHECK(length_to_resolve.IsFixed())
          << "We only support auto and fixed flex base sizes";
      flex_base_content_size = LayoutUnit(length_to_resolve.Value());
    }

    NGConstraintSpaceBuilder space_builder(ConstraintSpace());
    // TODO(dgrogan): Set the percentage size also.
    space_builder.SetAvailableSize(
        NGLogicalSize{container_logical_width, NGSizeIndefinite});
    scoped_refptr<NGConstraintSpace> child_space =
        space_builder.ToConstraintSpace(child.Style().GetWritingMode());

    LayoutUnit main_axis_border_and_padding =
        ComputeBorders(*child_space, child.Style()).InlineSum() +
        ComputePadding(*child_space, child.Style()).InlineSum();
    LayoutUnit main_axis_margin =
        ComputeMarginsForSelf(*child_space, child.Style()).InlineSum();

    // TODO(dgrogan): When child has a min/max-{width,height} set, call
    // Resolve{Inline,Block}Length here with child's style and constraint space.
    // Fill this in with the results.
    MinMaxSize min_max_sizes_in_main_axis_direction{LayoutUnit(),
                                                    LayoutUnit::Max()};
    flex_items.emplace_back(ToLayoutBox(Node().GetLayoutObject()),
                            flex_base_content_size,
                            min_max_sizes_in_main_axis_direction,
                            main_axis_border_and_padding, main_axis_margin);
    flex_items.back().ng_input_node = child;
  }

  FlexLayoutAlgorithm algorithm(&Style(), container_logical_width, flex_items);

  NGBoxStrut borders_scrollbar_padding =
      CalculateBorderScrollbarPadding(ConstraintSpace(), Node());
  LayoutUnit main_axis_offset = borders_scrollbar_padding.InlineSum();
  LayoutUnit cross_axis_offset = borders_scrollbar_padding.BlockSum();
  FlexLine* line;
  while ((line = algorithm.ComputeNextFlexLine(container_logical_width))) {
    line->SetContainerMainInnerSize(container_logical_width);
    line->FreezeInflexibleItems();
    while (!line->ResolveFlexibleLengths()) {
      continue;
    }
    for (size_t i = 0; i < line->line_items.size(); ++i) {
      FlexItem& flex_item = line->line_items[i];
      NGConstraintSpaceBuilder space_builder(ConstraintSpace());
      // TODO(dgrogan): Set the percentage size also.
      space_builder.SetAvailableSize(
          {flex_item.flexed_content_size, NGSizeIndefinite});
      space_builder.SetIsFixedSizeInline(true);
      scoped_refptr<NGConstraintSpace> child_space =
          space_builder.ToConstraintSpace(
              flex_item.box->Style()->GetWritingMode());
      flex_item.layout_result =
          flex_item.ng_input_node.Layout(*child_space, nullptr /*break token*/);
      flex_item.cross_axis_size =
          flex_item.layout_result->PhysicalFragment()->Size().height;
    }
    // cross_axis_offset is updated in each iteration of the loop, for passing
    // in to the next iteration.
    line->ComputeLineItemsPosition(main_axis_offset, cross_axis_offset);

    for (size_t i = 0; i < line->line_items.size(); ++i) {
      FlexItem& flex_item = line->line_items[i];
      container_builder_.AddChild(
          flex_item.layout_result,
          {flex_item.desired_location.X(), flex_item.desired_location.Y()});
    }
  }
  return container_builder_.ToBoxFragment();
}

base::Optional<MinMaxSize> NGFlexLayoutAlgorithm::ComputeMinMaxSize(
    const MinMaxSizeInput& input) const {
  // TODO(dgrogan): Implement this.
  return base::nullopt;
}

}  // namespace blink
