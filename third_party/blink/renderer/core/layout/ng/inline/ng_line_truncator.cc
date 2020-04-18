// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/inline/ng_line_truncator.h"

#include "third_party/blink/renderer/core/layout/ng/inline/ng_inline_item_result.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_text_fragment_builder.h"
#include "third_party/blink/renderer/platform/fonts/font_baseline.h"
#include "third_party/blink/renderer/platform/fonts/shaping/harf_buzz_shaper.h"

namespace blink {

namespace {

// Create the style to use for the ellipsis characters.
//
// The ellipsis is styled according to the line style.
// https://drafts.csswg.org/css-ui/#ellipsing-details
scoped_refptr<const ComputedStyle> CreateEllipsisStyle(
    scoped_refptr<const ComputedStyle> line_style) {
  if (line_style->TextDecorationsInEffect() == TextDecoration::kNone)
    return line_style;

  // Ellipsis should not have text decorations. Reset if it's set.
  // This is not defined, but 4 impls do this.
  scoped_refptr<ComputedStyle> ellipsis_style =
      ComputedStyle::CreateAnonymousStyleWithDisplay(*line_style,
                                                     EDisplay::kInline);
  ellipsis_style->ResetTextDecoration();
  ellipsis_style->ClearAppliedTextDecorations();
  return ellipsis_style;
}

}  // namespace

NGLineTruncator::NGLineTruncator(NGInlineNode& node,
                                 const NGLineInfo& line_info)
    : node_(node),
      line_style_(&line_info.LineStyle()),
      available_width_(line_info.AvailableWidth()),
      line_direction_(line_info.BaseDirection()) {}

LayoutUnit NGLineTruncator::TruncateLine(
    LayoutUnit line_width,
    NGLineBoxFragmentBuilder::ChildList* line_box) {
  // Shape the ellipsis and compute its inline size.
  scoped_refptr<const ComputedStyle> ellipsis_style =
      CreateEllipsisStyle(line_style_);
  const Font& font = ellipsis_style->GetFont();
  const SimpleFontData* font_data = font.PrimaryFont();
  DCHECK(font_data);
  String ellipsis_text =
      font_data && font_data->GlyphForCharacter(kHorizontalEllipsisCharacter)
          ? String(&kHorizontalEllipsisCharacter, 1)
          : String(u"...");
  HarfBuzzShaper shaper(ellipsis_text.Characters16(), ellipsis_text.length());
  scoped_refptr<ShapeResult> ellipsis_shape_result =
      shaper.Shape(&font, line_direction_);
  LayoutUnit ellipsis_width = ellipsis_shape_result->SnappedWidth();

  // Loop children from the logical last to the logical first to determine where
  // to place the ellipsis. Children maybe truncated or moved as part of the
  // process.
  LayoutUnit ellipsis_inline_offset;
  const NGPhysicalFragment* ellipsized_fragment = nullptr;
  if (IsLtr(line_direction_)) {
    NGLineBoxFragmentBuilder::Child* first_child = line_box->FirstInFlowChild();
    for (auto it = line_box->rbegin(); it != line_box->rend(); it++) {
      auto& child = *it;
      if (base::Optional<LayoutUnit> candidate = EllipsisOffset(
              line_width, ellipsis_width, &child == first_child, &child)) {
        ellipsis_inline_offset = candidate.value();
        ellipsized_fragment = child.PhysicalFragment();
        DCHECK(ellipsized_fragment);
        break;
      }
    }
  } else {
    NGLineBoxFragmentBuilder::Child* first_child = line_box->LastInFlowChild();
    ellipsis_inline_offset = available_width_ - ellipsis_width;
    for (auto& child : *line_box) {
      if (base::Optional<LayoutUnit> candidate = EllipsisOffset(
              line_width, ellipsis_width, &child == first_child, &child)) {
        ellipsis_inline_offset = candidate.value();
        ellipsized_fragment = child.PhysicalFragment();
        DCHECK(ellipsized_fragment);
        break;
      }
    }
  }

  // Abort if ellipsis could not be placed.
  if (!ellipsized_fragment)
    return line_width;

  // Now the offset of the ellpisis is determined. Place the ellpisis into the
  // line box.
  NGTextFragmentBuilder builder(node_, line_style_->GetWritingMode());
  DCHECK(ellipsized_fragment->GetLayoutObject() &&
         ellipsized_fragment->GetLayoutObject()->IsInline());
  builder.SetText(ellipsized_fragment->GetLayoutObject(), ellipsis_text,
                  ellipsis_style, true /* is_ellipsis_style */,
                  std::move(ellipsis_shape_result));
  FontBaseline baseline_type = line_style_->GetFontBaseline();
  NGLineHeightMetrics ellipsis_metrics(font_data->GetFontMetrics(),
                                       baseline_type);
  line_box->AddChild(
      builder.ToTextFragment(),
      NGLogicalOffset{ellipsis_inline_offset, -ellipsis_metrics.ascent},
      ellipsis_width, 0);
  return std::max(ellipsis_inline_offset + ellipsis_width, line_width);
}

// Return the offset to place the ellipsis.
//
// This function may truncate or move the child so that the ellipsis can fit.
base::Optional<LayoutUnit> NGLineTruncator::EllipsisOffset(
    LayoutUnit line_width,
    LayoutUnit ellipsis_width,
    bool is_first_child,
    NGLineBoxFragmentBuilder::Child* child) {
  // Leave out-of-flow children as is.
  if (!child->HasInFlowFragment())
    return base::nullopt;

  // Can't place ellipsis if this child is completely outside of the box.
  DCHECK_GE(line_width, child->offset.inline_offset + child->inline_size);
  LayoutUnit child_inline_offset =
      IsLtr(line_direction_)
          ? child->offset.inline_offset
          : line_width - (child->offset.inline_offset + child->inline_size);
  LayoutUnit space_for_child = available_width_ - child_inline_offset;
  if (space_for_child <= 0)
    return base::nullopt;

  // If not all of this child can fit, try to truncate.
  space_for_child -= ellipsis_width;
  if (space_for_child < child->inline_size &&
      !TruncateChild(space_for_child, is_first_child, child)) {
    // This child maybe partially visible. When it can't be truncated, move it
    // out so that none of this child should be visible.
    child->offset.inline_offset = line_width;
    return base::nullopt;
  }

  return IsLtr(line_direction_)
             ? child->offset.inline_offset + child->inline_size
             : child->offset.inline_offset - ellipsis_width;
}

// Truncate the specified child. Returns true if truncated successfully, false
// otherwise.
//
// Note that this function may return true even if it can't fit the child when
// |is_first_child|, because the spec defines that the first character or atomic
// inline-level element on a line must be clipped rather than ellipsed.
// https://drafts.csswg.org/css-ui/#text-overflow
bool NGLineTruncator::TruncateChild(LayoutUnit space_for_child,
                                    bool is_first_child,
                                    NGLineBoxFragmentBuilder::Child* child) {
  // If the space is not enough, try the next child.
  if (space_for_child <= 0 && !is_first_child)
    return false;

  // Only text fragments can be truncated.
  if (!child->fragment)
    return is_first_child;
  auto& fragment = ToNGPhysicalTextFragment(*child->fragment);
  const ShapeResult* shape_result = fragment.TextShapeResult();
  if (!shape_result)
    return is_first_child;

  // Compute the offset to truncate.
  unsigned new_length = shape_result->OffsetToFit(
      IsLtr(line_direction_) ? space_for_child
                             : shape_result->Width() - space_for_child,
      line_direction_);
  if (!new_length || new_length == fragment.Length()) {
    if (!is_first_child)
      return false;
    new_length = !new_length ? 1 : new_length - 1;
  }

  // Truncate the text fragment.
  child->fragment = line_direction_ == shape_result->Direction()
                        ? fragment.TrimText(fragment.StartOffset(),
                                            fragment.StartOffset() + new_length)
                        : fragment.TrimText(fragment.StartOffset() + new_length,
                                            fragment.EndOffset());
  LayoutUnit new_inline_size = line_style_->IsHorizontalWritingMode()
                                   ? child->fragment->Size().width
                                   : child->fragment->Size().height;
  DCHECK_LE(new_inline_size, child->inline_size);
  if (UNLIKELY(IsRtl(line_direction_)))
    child->offset.inline_offset += child->inline_size - new_inline_size;
  child->inline_size = new_inline_size;
  return true;
}

}  // namespace blink
