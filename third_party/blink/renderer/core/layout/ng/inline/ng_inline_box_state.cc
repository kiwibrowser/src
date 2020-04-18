// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/inline/ng_inline_box_state.h"

#include "third_party/blink/renderer/core/layout/ng/geometry/ng_logical_offset.h"
#include "third_party/blink/renderer/core/layout/ng/geometry/ng_logical_size.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_inline_item_result.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_line_box_fragment_builder.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_text_fragment_builder.h"
#include "third_party/blink/renderer/core/layout/ng/ng_fragment_builder.h"
#include "third_party/blink/renderer/core/layout/ng/ng_layout_result.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {

void NGInlineBoxState::ComputeTextMetrics(const ComputedStyle& style,
                                          FontBaseline baseline_type) {
  text_metrics = NGLineHeightMetrics(style, baseline_type);
  text_top = -text_metrics.ascent;
  text_height = text_metrics.LineHeight();
  text_metrics.AddLeading(style.ComputedLineHeightAsFixed());

  metrics.Unite(text_metrics);

  include_used_fonts = style.LineHeight().IsNegative();
}

void NGInlineBoxState::EnsureTextMetrics(const ComputedStyle& style,
                                         FontBaseline baseline_type) {
  if (text_metrics.IsEmpty())
    ComputeTextMetrics(style, baseline_type);
}

void NGInlineBoxState::AccumulateUsedFonts(const ShapeResult* shape_result,
                                           FontBaseline baseline_type) {
  HashSet<const SimpleFontData*> fallback_fonts;
  shape_result->FallbackFonts(&fallback_fonts);
  for (auto* const fallback_font : fallback_fonts) {
    NGLineHeightMetrics fallback_metrics(fallback_font->GetFontMetrics(),
                                         baseline_type);
    fallback_metrics.AddLeading(
        fallback_font->GetFontMetrics().FixedLineSpacing());
    metrics.Unite(fallback_metrics);
  }
}

bool NGInlineBoxState::CanAddTextOfStyle(
    const ComputedStyle& text_style) const {
  if (text_style.VerticalAlign() != EVerticalAlign::kBaseline)
    return false;
  DCHECK(style);
  if (style == &text_style || &style->GetFont() == &text_style.GetFont() ||
      style->GetFont().PrimaryFont() == text_style.GetFont().PrimaryFont())
    return true;
  return false;
}

LayoutObject*
NGInlineLayoutStateStack::ContainingLayoutObjectForAbsolutePositionObjects()
    const {
  for (unsigned i = stack_.size(); i-- > 1;) {
    const auto& box = stack_[i];
    DCHECK(box.style);
    if (box.style->CanContainAbsolutePositionObjects()) {
      DCHECK(box.item->GetLayoutObject());
      return box.item->GetLayoutObject();
    }
  }
  return nullptr;
}

NGInlineBoxState* NGInlineLayoutStateStack::OnBeginPlaceItems(
    const ComputedStyle* line_style,
    FontBaseline baseline_type,
    bool line_height_quirk) {
  if (stack_.IsEmpty()) {
    // For the first line, push a box state for the line itself.
    stack_.resize(1);
    NGInlineBoxState* box = &stack_.back();
    box->fragment_start = 0;
  } else {
    // For the following lines, clear states that are not shared across lines.
    for (auto& box : stack_) {
      box.fragment_start = 0;
      if (!line_height_quirk)
        box.metrics = box.text_metrics;
      else
        box.metrics = box.text_metrics = NGLineHeightMetrics();
      if (box.needs_box_fragment) {
        // Existing box states are wrapped before they were closed, and hence
        // they do not have start edges.
        box.has_start_edge = false;
        box.margin_inline_start = LayoutUnit();
        box.margin_border_padding_inline_start = LayoutUnit();
      }
      DCHECK(box.pending_descendants.IsEmpty());
    }
  }

  DCHECK(box_data_list_.IsEmpty());

  // Initialize the box state for the line box.
  NGInlineBoxState& line_box = LineBoxState();
  if (line_box.style != line_style) {
    line_box.style = line_style;

    // Use a "strut" (a zero-width inline box with the element's font and
    // line height properties) as the initial metrics for the line box.
    // https://drafts.csswg.org/css2/visudet.html#strut
    if (!line_height_quirk)
      line_box.ComputeTextMetrics(*line_style, baseline_type);
  }

  return &stack_.back();
}

NGInlineBoxState* NGInlineLayoutStateStack::OnOpenTag(
    const NGInlineItem& item,
    const NGInlineItemResult& item_result,
    const NGLineBoxFragmentBuilder::ChildList& line_box) {
  DCHECK(item.Style());
  NGInlineBoxState* box = OnOpenTag(*item.Style(), line_box);
  box->item = &item;

  // Compute box properties regardless of needs_box_fragment since close tag may
  // also set needs_box_fragment.
  box->padding = item_result.padding;
  box->has_start_edge = item_result.has_edge;
  if (box->has_start_edge) {
    box->margin_inline_start = item_result.margins.inline_start;
    // The open tag item has the start margin+border+padding in |inline_size|.
    box->margin_border_padding_inline_start = item_result.inline_size;
  } else {
    DCHECK_EQ(item_result.margins.inline_start, LayoutUnit());
    DCHECK_EQ(item_result.inline_size, LayoutUnit());
  }
  box->border_padding_block_start = item_result.borders_paddings_block_start;
  box->border_padding_block_end = item_result.borders_paddings_block_end;
  return box;
}

NGInlineBoxState* NGInlineLayoutStateStack::OnOpenTag(
    const ComputedStyle& style,
    const NGLineBoxFragmentBuilder::ChildList& line_box) {
  stack_.resize(stack_.size() + 1);
  NGInlineBoxState* box = &stack_.back();
  box->fragment_start = line_box.size();
  box->style = &style;
  return box;
}

NGInlineBoxState* NGInlineLayoutStateStack::OnCloseTag(
    NGLineBoxFragmentBuilder::ChildList* line_box,
    NGInlineBoxState* box,
    FontBaseline baseline_type) {
  EndBoxState(box, line_box, baseline_type);
  // TODO(kojii): When the algorithm restarts from a break token, the stack may
  // underflow. We need either synthesize a missing box state, or push all
  // parents on initialize.
  stack_.pop_back();
  return &stack_.back();
}

void NGInlineLayoutStateStack::OnEndPlaceItems(
    NGLineBoxFragmentBuilder::ChildList* line_box,
    FontBaseline baseline_type) {
  for (auto it = stack_.rbegin(); it != stack_.rend(); ++it) {
    NGInlineBoxState* box = &(*it);
    EndBoxState(box, line_box, baseline_type);
  }
}

void NGInlineLayoutStateStack::EndBoxState(
    NGInlineBoxState* box,
    NGLineBoxFragmentBuilder::ChildList* line_box,
    FontBaseline baseline_type) {
  if (box->needs_box_fragment)
    AddBoxFragmentPlaceholder(box, line_box, baseline_type);

  PositionPending position_pending =
      ApplyBaselineShift(box, line_box, baseline_type);

  // Unite the metrics to the parent box.
  if (position_pending == kPositionNotPending && box != stack_.begin()) {
    box[-1].metrics.Unite(box->metrics);
  }
}

void NGInlineBoxState::SetNeedsBoxFragment() {
  DCHECK(item);
  needs_box_fragment = true;
}

void NGInlineBoxState::SetLineRightForBoxFragment(
    const NGInlineItem& item,
    const NGInlineItemResult& item_result) {
  DCHECK(needs_box_fragment);
  has_end_edge = item_result.has_edge;
  if (has_end_edge) {
    margin_inline_end = item_result.margins.inline_end;
    // The close tag item has the end margin+border+padding in |inline_size|.
    margin_border_padding_inline_end = item_result.inline_size;
  } else {
    DCHECK_EQ(item_result.margins.inline_end, LayoutUnit());
    DCHECK_EQ(item_result.inline_size, LayoutUnit());
  }
}

// Crete a placeholder for a box fragment.
// We keep a flat list of fragments because it is more suitable for operations
// such as ApplyBaselineShift. Later, CreateBoxFragments() creates box fragments
// from placeholders.
void NGInlineLayoutStateStack::AddBoxFragmentPlaceholder(
    NGInlineBoxState* box,
    NGLineBoxFragmentBuilder::ChildList* line_box,
    FontBaseline baseline_type) {
  DCHECK(box->needs_box_fragment);

  // The inline box should have the height of the font metrics without the
  // line-height property. Compute from style because |box->metrics| includes
  // the line-height property.
  DCHECK(box->style);
  const ComputedStyle& style = *box->style;
  NGLineHeightMetrics metrics(style, baseline_type);

  // Extend the block direction of the box by borders and paddings. Inline
  // direction is already included into positions in NGLineBreaker.
  NGLogicalOffset offset(LayoutUnit(),
                         -metrics.ascent - box->border_padding_block_start);
  NGLogicalSize size(LayoutUnit(), metrics.LineHeight() +
                                       box->border_padding_block_start +
                                       box->border_padding_block_end);

  unsigned fragment_end = line_box->size();
  DCHECK(box->item);
  box_data_list_.push_back(
      BoxData{box->fragment_start, fragment_end, box->item, size});
  BoxData& box_data = box_data_list_.back();
  box_data.padding = box->padding;
  if (box->has_start_edge) {
    box_data.has_line_left_edge = true;
    box_data.margin_line_left = box->margin_inline_start;
    box_data.margin_border_padding_line_left =
        box->margin_border_padding_inline_start;
  }
  if (box->has_end_edge) {
    box_data.has_line_right_edge = true;
    box_data.margin_line_right = box->margin_inline_end;
    box_data.margin_border_padding_line_right =
        box->margin_border_padding_inline_end;
  }
  if (IsRtl(style.Direction())) {
    std::swap(box_data.has_line_left_edge, box_data.has_line_right_edge);
    std::swap(box_data.margin_line_left, box_data.margin_line_right);
    std::swap(box_data.margin_border_padding_line_left,
              box_data.margin_border_padding_line_right);
  }

  if (fragment_end > box->fragment_start) {
    // The start is marked only in BoxData, while end is marked
    // in both BoxData and the list itself.
    // With a list of 4 text fragments:
    // |  0  |  1  |  2  |  3  |
    // |text0|text1|text2|text3|
    // By adding a BoxData(2,4) (end is exclusive), it becomes:
    // |  0  |  1  |  2  |  3  |  4  |
    // |text0|text1|text2|text3|null |
    // The "null" is added to the list to compute baseline shift of the box
    // separately from text fragments.
    line_box->AddChild(offset);
  } else {
    // Do not defer creating a box fragment if this is an empty inline box.
    // An empty box fragment is still flat that we do not have to defer.
    // Also, placeholders cannot be reordred if empty.
    offset.inline_offset += box_data.margin_line_left;
    LayoutUnit advance = box_data.margin_border_padding_line_left +
                         box_data.margin_border_padding_line_right;
    box_data.size.inline_size =
        advance - box_data.margin_line_left - box_data.margin_line_right;
    line_box->AddChild(box_data.CreateBoxFragment(line_box), offset, advance,
                       0);
    box_data_list_.pop_back();
  }
}

void NGInlineLayoutStateStack::PrepareForReorder(
    NGLineBoxFragmentBuilder::ChildList* line_box) {
  // Set indexes of BoxData to the children of the line box.
  unsigned box_data_index = 0;
  for (const auto& box_data : box_data_list_) {
    box_data_index++;
    for (unsigned i = box_data.fragment_start; i < box_data.fragment_end; i++) {
      NGLineBoxFragmentBuilder::Child& child = (*line_box)[i];
      if (!child.box_data_index)
        child.box_data_index = box_data_index;
    }
  }

  // When boxes are nested, placeholders have indexes to which box it should be
  // added. Copy them to BoxData.
  for (auto& box_data : box_data_list_) {
    const NGLineBoxFragmentBuilder::Child& placeholder =
        (*line_box)[box_data.fragment_end];
    DCHECK(!placeholder.HasFragment());
    box_data.offset = placeholder.offset;
    box_data.box_data_index = placeholder.box_data_index;
  }
}

void NGInlineLayoutStateStack::UpdateAfterReorder(
    NGLineBoxFragmentBuilder::ChildList* line_box) {
  // Compute start/end of boxes from the children of the line box.
  for (auto& box_data : box_data_list_)
    box_data.fragment_start = box_data.fragment_end = 0;
  for (unsigned i = 0; i < line_box->size(); i++) {
    const auto& child = (*line_box)[i];
    if (!child.HasFragment())
      continue;
    if (unsigned box_data_index = child.box_data_index) {
      BoxData& box_data = box_data_list_[box_data_index - 1];
      if (!box_data.fragment_end)
        box_data.fragment_start = i;
      box_data.fragment_end = i + 1;
    }
  }

  // Extend start/end of boxes when they are nested.
  for (auto& box_data : box_data_list_) {
    if (box_data.box_data_index) {
      BoxData& parent_box_data = box_data_list_[box_data.box_data_index - 1];
      if (!parent_box_data.fragment_end) {
        parent_box_data.fragment_start = box_data.fragment_start;
        parent_box_data.fragment_end = box_data.fragment_end;
      } else {
        parent_box_data.fragment_start =
            std::min(box_data.fragment_start, parent_box_data.fragment_start);
        parent_box_data.fragment_end =
            std::max(box_data.fragment_end, parent_box_data.fragment_end);
      }
    }
  }

#if DCHECK_IS_ON()
  // Check all BoxData have ranges.
  for (const auto& box_data : box_data_list_) {
    DCHECK_NE(box_data.fragment_end, 0u);
    DCHECK_GT(box_data.fragment_end, box_data.fragment_start);
  }
#endif
}

LayoutUnit NGInlineLayoutStateStack::ComputeInlinePositions(
    NGLineBoxFragmentBuilder::ChildList* line_box) {
  // At this point, children are in the visual order, and they have their
  // origins at (0, 0). Accumulate inline offset from left to right.
  LayoutUnit position;
  for (auto& child : *line_box) {
    child.offset.inline_offset += position;
    // Box margins/boders/paddings will be processed later.
    // TODO(kojii): we could optimize this if the reordering did not occur.
    if (!child.HasFragment())
      continue;
    position += child.inline_size;
  }

  if (box_data_list_.IsEmpty())
    return position;

  // Compute inline positions of inline boxes.
  for (auto& box_data : box_data_list_) {
    unsigned start = box_data.fragment_start;
    unsigned end = box_data.fragment_end;
    DCHECK_GT(end, start);
    NGLineBoxFragmentBuilder::Child& start_child = (*line_box)[start];

    // Clamping left offset is not defined, match to the existing behavior.
    LayoutUnit line_left_offset =
        start_child.offset.inline_offset.ClampNegativeToZero();
    LayoutUnit line_right_offset = end < line_box->size()
                                       ? (*line_box)[end].offset.inline_offset
                                       : position;
    box_data.offset.inline_offset =
        line_left_offset + box_data.margin_line_left;
    box_data.size.inline_size =
        line_right_offset - line_left_offset +
        box_data.margin_border_padding_line_left - box_data.margin_line_left +
        box_data.margin_border_padding_line_right - box_data.margin_line_right;

    // Adjust child offsets for margin/border/padding.
    if (box_data.margin_border_padding_line_left) {
      line_box->MoveInInlineDirection(box_data.margin_border_padding_line_left,
                                      start, line_box->size());
      position += box_data.margin_border_padding_line_left;
    }

    if (box_data.margin_border_padding_line_right) {
      line_box->MoveInInlineDirection(box_data.margin_border_padding_line_right,
                                      end, line_box->size());
      position += box_data.margin_border_padding_line_right;
    }
  }

  return position;
}

void NGInlineLayoutStateStack::CreateBoxFragments(
    NGLineBoxFragmentBuilder::ChildList* line_box) {
  DCHECK(!box_data_list_.IsEmpty());

  for (auto& box_data : box_data_list_) {
    unsigned start = box_data.fragment_start;
    unsigned end = box_data.fragment_end;
    DCHECK_GT(end, start);
    NGLineBoxFragmentBuilder::Child& start_child = (*line_box)[start];

    scoped_refptr<NGLayoutResult> box_fragment =
        box_data.CreateBoxFragment(line_box);
    if (!start_child.HasFragment()) {
      start_child.layout_result = std::move(box_fragment);
      start_child.offset = box_data.offset;
    } else {
      // In most cases, |start_child| is moved to the children of the box, and
      // is empty. It's not empty when it's out-of-flow. Insert in such case.
      line_box->InsertChild(start, std::move(box_fragment), box_data.offset,
                            LayoutUnit(), 0);
    }
  }

  box_data_list_.clear();
}

scoped_refptr<NGLayoutResult>
NGInlineLayoutStateStack::BoxData::CreateBoxFragment(
    NGLineBoxFragmentBuilder::ChildList* line_box) {
  DCHECK(item);
  DCHECK(item->Style());
  const ComputedStyle& style = *item->Style();
  // Because children are already in the visual order, use LTR for the
  // fragment builder so that it should not transform the coordinates for RTL.
  NGFragmentBuilder box(item->GetLayoutObject(), &style, style.GetWritingMode(),
                        TextDirection::kLtr);
  box.SetBoxType(NGPhysicalFragment::kInlineBox);
  box.SetStyleVariant(item->StyleVariant());

  // Inline boxes have block start/end borders, even when its containing block
  // was fragmented. Fragmenting a line box in block direction is not
  // supported today.
  box.SetBorderEdges({true, has_line_right_edge, true, has_line_left_edge});
  box.SetInlineSize(size.inline_size.ClampNegativeToZero());
  box.SetBlockSize(size.block_size);
  box.SetPadding(padding);

  for (unsigned i = fragment_start; i < fragment_end; i++) {
    NGLineBoxFragmentBuilder::Child& child = (*line_box)[i];
    if (child.layout_result) {
      box.AddChild(std::move(child.layout_result), child.offset - offset);
    } else if (child.fragment) {
      box.AddChild(std::move(child.fragment), child.offset - offset);
    }
    // Leave out-of-flow fragments. They need to be at the top level so that
    // NGInlineLayoutAlgorithm can handle them later.
    DCHECK(!child.HasInFlowFragment());
  }

  return box.ToBoxFragment();
}

NGInlineLayoutStateStack::PositionPending
NGInlineLayoutStateStack::ApplyBaselineShift(
    NGInlineBoxState* box,
    NGLineBoxFragmentBuilder::ChildList* line_box,
    FontBaseline baseline_type) {
  // Some 'vertical-align' values require the size of their parents. Align all
  // such descendant boxes that require the size of this box; they are queued in
  // |pending_descendants|.
  LayoutUnit baseline_shift;
  if (!box->pending_descendants.IsEmpty()) {
    for (auto& child : box->pending_descendants) {
      if (child.metrics.IsEmpty()) {
        // This can happen with boxes with no content in quirks mode
        child.metrics = NGLineHeightMetrics(LayoutUnit(), LayoutUnit());
      }
      switch (child.vertical_align) {
        case EVerticalAlign::kTextTop:
          DCHECK(!box->text_metrics.IsEmpty());
          baseline_shift = child.metrics.ascent + box->text_top;
          break;
        case EVerticalAlign::kTop:
          if (box->metrics.IsEmpty())
            baseline_shift = child.metrics.ascent;
          else
            baseline_shift = child.metrics.ascent - box->metrics.ascent;
          break;
        case EVerticalAlign::kTextBottom:
          if (const SimpleFontData* font_data =
                  box->style->GetFont().PrimaryFont()) {
            LayoutUnit text_bottom =
                font_data->GetFontMetrics().FixedDescent(baseline_type);
            baseline_shift = text_bottom - child.metrics.descent;
            break;
          }
          NOTREACHED();
          FALLTHROUGH;
        case EVerticalAlign::kBottom:
          if (box->metrics.IsEmpty())
            baseline_shift = -child.metrics.descent;
          else
            baseline_shift = box->metrics.descent - child.metrics.descent;
          break;
        default:
          NOTREACHED();
          continue;
      }
      child.metrics.Move(baseline_shift);
      box->metrics.Unite(child.metrics);
      line_box->MoveInBlockDirection(baseline_shift, child.fragment_start,
                                     child.fragment_end);
    }
    box->pending_descendants.clear();
  }

  const ComputedStyle& style = *box->style;
  EVerticalAlign vertical_align = style.VerticalAlign();
  if (vertical_align == EVerticalAlign::kBaseline)
    return kPositionNotPending;

  // 'vertical-align' aligns boxes relative to themselves, to their parent
  // boxes, or to the line box, depends on the value.
  // Because |box| is an item in |stack_|, |box[-1]| is its parent box.
  // If this box doesn't have a parent; i.e., this box is a line box,
  // 'vertical-align' has no effect.
  DCHECK(box >= stack_.begin() && box < stack_.end());
  if (box == stack_.begin())
    return kPositionNotPending;
  NGInlineBoxState& parent_box = box[-1];

  // Check if there are any fragments to move.
  unsigned fragment_end = line_box->size();
  if (box->fragment_start == fragment_end)
    return kPositionNotPending;

  switch (vertical_align) {
    case EVerticalAlign::kSub:
      baseline_shift = parent_box.style->ComputedFontSizeAsFixed() / 5 + 1;
      break;
    case EVerticalAlign::kSuper:
      baseline_shift = -(parent_box.style->ComputedFontSizeAsFixed() / 3 + 1);
      break;
    case EVerticalAlign::kLength: {
      // 'Percentages: refer to the 'line-height' of the element itself'.
      // https://www.w3.org/TR/CSS22/visudet.html#propdef-vertical-align
      const Length& length = style.GetVerticalAlignLength();
      LayoutUnit line_height = length.IsPercentOrCalc()
                                   ? style.ComputedLineHeightAsFixed()
                                   : box->text_metrics.LineHeight();
      baseline_shift = -ValueForLength(length, line_height);
      break;
    }
    case EVerticalAlign::kMiddle:
      baseline_shift = (box->metrics.ascent - box->metrics.descent) / 2;
      if (const SimpleFontData* parent_font_data =
              parent_box.style->GetFont().PrimaryFont()) {
        baseline_shift -= LayoutUnit::FromFloatRound(
            parent_font_data->GetFontMetrics().XHeight() / 2);
      }
      break;
    case EVerticalAlign::kBaselineMiddle:
      baseline_shift = (box->metrics.ascent - box->metrics.descent) / 2;
      break;
    case EVerticalAlign::kTop:
    case EVerticalAlign::kBottom:
      // 'top' and 'bottom' require the layout size of the line box.
      stack_.front().pending_descendants.push_back(NGPendingPositions{
          box->fragment_start, fragment_end, box->metrics, vertical_align});
      return kPositionPending;
    default:
      // Other values require the layout size of the parent box.
      parent_box.pending_descendants.push_back(NGPendingPositions{
          box->fragment_start, fragment_end, box->metrics, vertical_align});
      return kPositionPending;
  }
  if (!box->metrics.IsEmpty())
    box->metrics.Move(baseline_shift);
  line_box->MoveInBlockDirection(baseline_shift, box->fragment_start,
                                 fragment_end);
  return kPositionNotPending;
}

}  // namespace blink
