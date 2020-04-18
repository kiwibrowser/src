// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/inline/ng_inline_item_result.h"

#include "third_party/blink/renderer/core/layout/ng/inline/ng_inline_node.h"
#include "third_party/blink/renderer/core/layout/ng/ng_constraint_space.h"

namespace blink {

NGInlineItemResult::NGInlineItemResult()
    : item(nullptr), item_index(0), start_offset(0), end_offset(0) {}

NGInlineItemResult::NGInlineItemResult(const NGInlineItem* item,
                                       unsigned index,
                                       unsigned start,
                                       unsigned end)
    : item(item), item_index(index), start_offset(start), end_offset(end) {}

void NGLineInfo::SetLineStyle(const NGInlineNode& node,
                              const NGInlineItemsData& items_data,
                              const NGConstraintSpace& constraint_space,
                              bool is_first_line,
                              bool use_first_line_style,
                              bool is_after_forced_break) {
  use_first_line_style_ = use_first_line_style;
  items_data_ = &items_data;
  line_style_ = node.GetLayoutObject()->Style(use_first_line_style_);

  if (line_style_->ShouldUseTextIndent(is_first_line, is_after_forced_break)) {
    // 'text-indent' applies to block container, and percentage is of its
    // containing block.
    // https://drafts.csswg.org/css-text-3/#valdef-text-indent-percentage
    // In our constraint space tree, parent constraint space is of its
    // containing block.
    // TODO(kojii): ComputeMinMaxSize does not know parent constraint
    // space that we cannot compute percent for text-indent.
    const Length& length = line_style_->TextIndent();
    LayoutUnit maximum_value;
    if (length.IsPercentOrCalc())
      maximum_value = constraint_space.ParentPercentageResolutionInlineSize();
    text_indent_ = MinimumValueForLength(length, maximum_value);
  } else {
    text_indent_ = LayoutUnit();
  }
}

#if DCHECK_IS_ON()
void NGInlineItemResult::CheckConsistency() const {
  DCHECK(item);
  if (item->Type() == NGInlineItem::kText) {
    DCHECK(shape_result);
    DCHECK_LT(start_offset, end_offset);
    DCHECK_EQ(end_offset - start_offset, shape_result->NumCharacters());
    DCHECK_EQ(start_offset, shape_result->StartIndexForResult());
    DCHECK_EQ(end_offset, shape_result->EndIndexForResult());
  }
}
#endif

void NGLineInfo::SetLineBfcOffset(NGBfcOffset line_bfc_offset,
                                  LayoutUnit available_width,
                                  LayoutUnit width) {
  line_bfc_offset_ = line_bfc_offset;
  available_width_ = available_width;
  width_ = width;
}

void NGLineInfo::SetLineEndFragment(
    scoped_refptr<NGPhysicalTextFragment> fragment) {
  line_end_fragment_ = std::move(fragment);
}

}  // namespace blink
