// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/inline/ng_physical_text_fragment.h"

#include "third_party/blink/renderer/core/dom/node.h"
#include "third_party/blink/renderer/core/layout/layout_text_fragment.h"
#include "third_party/blink/renderer/core/layout/line/line_orientation_utils.h"
#include "third_party/blink/renderer/core/layout/ng/geometry/ng_physical_offset_rect.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_inline_item.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {

// Convert logical cooridnate to local physical coordinate.
NGPhysicalOffsetRect NGPhysicalTextFragment::ConvertToLocal(
    const LayoutRect& logical_rect) const {
  switch (LineOrientation()) {
    case NGLineOrientation::kHorizontal:
      return NGPhysicalOffsetRect(logical_rect);
    case NGLineOrientation::kClockWiseVertical:
      return {{size_.width - logical_rect.MaxY(), logical_rect.X()},
              {logical_rect.Height(), logical_rect.Width()}};
    case NGLineOrientation::kCounterClockWiseVertical:
      return {{logical_rect.Y(), size_.height - logical_rect.MaxX()},
              {logical_rect.Height(), logical_rect.Width()}};
  }
  NOTREACHED();
  return NGPhysicalOffsetRect(logical_rect);
}

// Compute the inline position from text offset, in logical coordinate relative
// to this fragment.
LayoutUnit NGPhysicalTextFragment::InlinePositionForOffset(
    unsigned offset,
    LayoutUnit (*round)(float),
    AdjustMidCluster adjust_mid_cluster) const {
  DCHECK_GE(offset, start_offset_);
  DCHECK_LE(offset, end_offset_);

  offset -= start_offset_;
  if (shape_result_) {
    return round(shape_result_->PositionForOffset(offset, adjust_mid_cluster));
  }

  // This fragment is a flow control because otherwise ShapeResult exists.
  DCHECK(IsFlowControl());
  DCHECK_EQ(1u, Length());
  if (!offset || UNLIKELY(IsRtl(Style().Direction())))
    return LayoutUnit();
  return IsHorizontal() ? Size().width : Size().height;
}

LayoutUnit NGPhysicalTextFragment::InlinePositionForOffset(
    unsigned offset) const {
  return InlinePositionForOffset(offset, LayoutUnit::FromFloatRound,
                                 AdjustMidCluster::kToEnd);
}

NGPhysicalOffsetRect NGPhysicalTextFragment::LocalRect(
    unsigned start_offset,
    unsigned end_offset) const {
  DCHECK_LE(start_offset, end_offset);
  DCHECK_GE(start_offset, start_offset_);
  DCHECK_LE(end_offset, end_offset_);

  LayoutUnit start_position = InlinePositionForOffset(
      start_offset, LayoutUnit::FromFloatFloor, AdjustMidCluster::kToStart);
  LayoutUnit end_position = InlinePositionForOffset(
      end_offset, LayoutUnit::FromFloatCeil, AdjustMidCluster::kToEnd);

  // Swap positions if RTL.
  if (UNLIKELY(start_position > end_position))
    std::swap(start_position, end_position);

  LayoutUnit inline_size = end_position - start_position;

  switch (LineOrientation()) {
    case NGLineOrientation::kHorizontal:
      return {{start_position, LayoutUnit()}, {inline_size, Size().height}};
    case NGLineOrientation::kClockWiseVertical:
      return {{LayoutUnit(), start_position}, {Size().width, inline_size}};
    case NGLineOrientation::kCounterClockWiseVertical:
      return {{LayoutUnit(), Size().height - end_position},
              {Size().width, inline_size}};
  }
  NOTREACHED();
  return {};
}

NGPhysicalOffsetRect NGPhysicalTextFragment::SelfVisualRect() const {
  if (UNLIKELY(!shape_result_))
    return LocalRect();

  // Glyph bounds is in logical coordinate, origin at the alphabetic baseline.
  LayoutRect visual_rect = EnclosingLayoutRect(shape_result_->Bounds());

  // Make the origin at the logical top of this fragment.
  const ComputedStyle& style = Style();
  const Font& font = style.GetFont();
  if (const SimpleFontData* font_data = font.PrimaryFont()) {
    visual_rect.SetY(visual_rect.Y() + font_data->GetFontMetrics().FixedAscent(
                                           kAlphabeticBaseline));
  }

  if (float stroke_width = style.TextStrokeWidth()) {
    visual_rect.Inflate(LayoutUnit::FromFloatCeil(stroke_width / 2.0f));
  }

  if (style.GetTextEmphasisMark() != TextEmphasisMark::kNone) {
    LayoutUnit emphasis_mark_height =
        LayoutUnit(font.EmphasisMarkHeight(style.TextEmphasisMarkString()));
    DCHECK_GT(emphasis_mark_height, LayoutUnit());
    if (style.GetTextEmphasisLineLogicalSide() == LineLogicalSide::kOver) {
      visual_rect.ShiftYEdgeTo(
          std::min(visual_rect.Y(), -emphasis_mark_height));
    } else {
      LayoutUnit logical_height =
          style.IsHorizontalWritingMode() ? Size().height : Size().width;
      visual_rect.ShiftMaxYEdgeTo(
          std::max(visual_rect.MaxY(), logical_height + emphasis_mark_height));
    }
  }

  if (ShadowList* text_shadow = style.TextShadow()) {
    LayoutRectOutsets text_shadow_logical_outsets =
        LineOrientationLayoutRectOutsets(
            LayoutRectOutsets(text_shadow->RectOutsetsIncludingOriginal()),
            style.GetWritingMode());
    text_shadow_logical_outsets.ClampNegativeToZero();
    visual_rect.Expand(text_shadow_logical_outsets);
  }

  visual_rect = LayoutRect(EnclosingIntRect(visual_rect));

  // Uniting the frame rect ensures that non-ink spaces such side bearings, or
  // even space characters, are included in the visual rect for decorations.
  NGPhysicalOffsetRect local_visual_rect = ConvertToLocal(visual_rect);
  local_visual_rect.Unite(LocalRect());
  return local_visual_rect;
}

scoped_refptr<NGPhysicalFragment> NGPhysicalTextFragment::TrimText(
    unsigned new_start_offset,
    unsigned new_end_offset) const {
  DCHECK(shape_result_);
  DCHECK_GE(new_start_offset, StartOffset());
  DCHECK_GT(new_end_offset, new_start_offset);
  DCHECK_LE(new_end_offset, EndOffset());
  scoped_refptr<ShapeResult> new_shape_result =
      shape_result_->SubRange(new_start_offset, new_end_offset);
  LayoutUnit new_inline_size = new_shape_result->SnappedWidth();
  return base::AdoptRef(new NGPhysicalTextFragment(
      layout_object_, Style(), static_cast<NGStyleVariant>(style_variant_),
      TextType(), text_, new_start_offset, new_end_offset,
      IsHorizontal() ? NGPhysicalSize{new_inline_size, size_.height}
                     : NGPhysicalSize{size_.width, new_inline_size},
      LineOrientation(), EndEffect(), std::move(new_shape_result)));
}

scoped_refptr<NGPhysicalFragment> NGPhysicalTextFragment::CloneWithoutOffset()
    const {
  return base::AdoptRef(new NGPhysicalTextFragment(
      layout_object_, Style(), static_cast<NGStyleVariant>(style_variant_),
      TextType(), text_, start_offset_, end_offset_, size_, LineOrientation(),
      EndEffect(), shape_result_));
}

bool NGPhysicalTextFragment::IsAnonymousText() const {
  // TODO(xiaochengh): Introduce and set a flag for anonymous text.
  const LayoutObject* layout_object = GetLayoutObject();
  if (layout_object && layout_object->IsText() &&
      ToLayoutText(layout_object)->IsTextFragment())
    return !ToLayoutTextFragment(layout_object)->AssociatedTextNode();
  const Node* node = GetNode();
  return !node || node->IsPseudoElement();
}

unsigned NGPhysicalTextFragment::TextOffsetForPoint(
    const NGPhysicalOffset& point) const {
  if (IsLineBreak())
    return StartOffset();
  DCHECK(TextShapeResult());
  const LayoutUnit& point_in_line_direction =
      Style().IsHorizontalWritingMode() ? point.left : point.top;
  const bool include_partial_glyphs = true;
  return TextShapeResult()->OffsetForPosition(point_in_line_direction.ToFloat(),
                                              include_partial_glyphs) +
         StartOffset();
}

UBiDiLevel NGPhysicalTextFragment::BidiLevel() const {
  // TODO(xiaochengh): Make the implementation more efficient with, e.g.,
  // binary search and/or LayoutNGText::InlineItems().
  const auto& items = InlineItemsOfContainingBlock();
  const NGInlineItem* containing_item = std::find_if(
      items.begin(), items.end(), [this](const NGInlineItem& item) {
        return item.StartOffset() <= StartOffset() &&
               item.EndOffset() >= EndOffset();
      });
  DCHECK(containing_item);
  DCHECK_NE(containing_item, items.end());
  return containing_item->BidiLevel();
}

TextDirection NGPhysicalTextFragment::ResolvedDirection() const {
  if (TextShapeResult())
    return TextShapeResult()->Direction();
  return NGPhysicalFragment::ResolvedDirection();
}

}  // namespace blink
