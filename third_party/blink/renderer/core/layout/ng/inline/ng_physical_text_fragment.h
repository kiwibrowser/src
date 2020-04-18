// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGPhysicalTextFragment_h
#define NGPhysicalTextFragment_h

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_text_end_effect.h"
#include "third_party/blink/renderer/core/layout/ng/ng_physical_fragment.h"
#include "third_party/blink/renderer/platform/fonts/ng_text_fragment_paint_info.h"
#include "third_party/blink/renderer/platform/fonts/shaping/shape_result.h"
#include "third_party/blink/renderer/platform/wtf/text/string_view.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

struct NGPhysicalOffsetRect;

enum class AdjustMidCluster;

// In CSS Writing Modes Levle 4, line orientation for layout and line
// orientation for paint are not always the same.
//
// Specifically, 'sideways-lr' typesets as if lines are horizontal flow, but
// rotates counterclockwise.
enum class NGLineOrientation {
  // Lines are horizontal.
  kHorizontal,
  // Lines are vertical, rotated clockwise. Inside of the line, it may be
  // typeset using vertical characteristics, horizontal characteristics, or
  // mixed. Lines flow left to right, or right to left.
  kClockWiseVertical,
  // Lines are vertical, rotated counterclockwise. Inside of the line is typeset
  // as if horizontal flow. Lines flow left to right.
  kCounterClockWiseVertical

  // When adding new values, ensure NGPhysicalTextFragment has enough bits.
};

class CORE_EXPORT NGPhysicalTextFragment final : public NGPhysicalFragment {
 public:
  enum NGTextType {
    kNormalText,
    kForcedLineBreak,
    // Flow controls are not to be painted. In particular, a tabulation
    // character and a soft-wrap opportunity.
    kFlowControl,
    // When adding new values, make sure the bit size of |sub_type_| is large
    // enough to store.
  };

  NGPhysicalTextFragment(LayoutObject* layout_object,
                         const ComputedStyle& style,
                         NGStyleVariant style_variant,
                         NGTextType text_type,
                         const String& text,
                         unsigned start_offset,
                         unsigned end_offset,
                         NGPhysicalSize size,
                         NGLineOrientation line_orientation,
                         NGTextEndEffect end_effect,
                         scoped_refptr<const ShapeResult> shape_result)
      : NGPhysicalFragment(layout_object,
                           style,
                           style_variant,
                           size,
                           kFragmentText,
                           text_type),
        text_(text),
        start_offset_(start_offset),
        end_offset_(end_offset),
        shape_result_(shape_result),
        line_orientation_(static_cast<unsigned>(line_orientation)),
        end_effect_(static_cast<unsigned>(end_effect)) {
    DCHECK(shape_result_ || IsFlowControl()) << ToString();
  }

  NGTextType TextType() const { return static_cast<NGTextType>(sub_type_); }
  // True if this is a forced line break.
  bool IsLineBreak() const { return TextType() == kForcedLineBreak; }
  // True if this is not for painting; i.e., a forced line break, a tabulation,
  // or a soft-wrap opportunity.
  bool IsFlowControl() const {
    return IsLineBreak() || TextType() == kFlowControl;
  }

  unsigned Length() const { return end_offset_ - start_offset_; }
  StringView Text() const { return StringView(text_, start_offset_, Length()); }

  // ShapeResult may be nullptr if |IsFlowControl()|.
  const ShapeResult* TextShapeResult() const { return shape_result_.get(); }

  // Start/end offset to the text of the block container.
  unsigned StartOffset() const { return start_offset_; }
  unsigned EndOffset() const { return end_offset_; }

  NGLineOrientation LineOrientation() const {
    return static_cast<NGLineOrientation>(line_orientation_);
  }
  bool IsHorizontal() const {
    return LineOrientation() == NGLineOrientation::kHorizontal;
  }

  // Compute the inline position from text offset, in logical coordinate
  // relative to this fragment.
  LayoutUnit InlinePositionForOffset(unsigned offset) const;

  // The layout box of text in (start, end) range in local coordinate.
  // Start and end offsets must be between StartOffset() and EndOffset().
  NGPhysicalOffsetRect LocalRect(unsigned start_offset,
                                 unsigned end_offset) const;
  using NGPhysicalFragment::LocalRect;

  // The visual bounding box that includes glpyh bounding box and CSS
  // properties, in local coordinates.
  NGPhysicalOffsetRect SelfVisualRect() const;

  NGTextEndEffect EndEffect() const {
    return static_cast<NGTextEndEffect>(end_effect_);
  }

  // Create a new fragment that has part of the text of this fragment.
  // All other properties are the same as this fragment.
  scoped_refptr<NGPhysicalFragment> TrimText(unsigned start_offset,
                                             unsigned end_offset) const;

  scoped_refptr<NGPhysicalFragment> CloneWithoutOffset() const;

  NGTextFragmentPaintInfo PaintInfo() const {
    return NGTextFragmentPaintInfo{text_, StartOffset(), EndOffset(),
                                   TextShapeResult()};
  }

  // Returns true if the text is generated (from, e.g., list marker,
  // pseudo-element, ...) instead of from a DOM text node.
  bool IsAnonymousText() const;

  // Returns the text offset in the fragment placed closest to the given point.
  unsigned TextOffsetForPoint(const NGPhysicalOffset&) const;

  UBiDiLevel BidiLevel() const override;
  TextDirection ResolvedDirection() const override;

 private:
  LayoutUnit InlinePositionForOffset(unsigned offset,
                                     LayoutUnit (*round)(float),
                                     AdjustMidCluster) const;

  NGPhysicalOffsetRect ConvertToLocal(const LayoutRect&) const;

  // The text of NGInlineNode; i.e., of a parent block. The text for this
  // fragment is a substring(start_offset_, end_offset_) of this string.
  const String text_;

  // Start and end offset of the parent block text.
  unsigned start_offset_;
  unsigned end_offset_;

  scoped_refptr<const ShapeResult> shape_result_;

  unsigned line_orientation_ : 2;  // NGLineOrientation
  unsigned end_effect_ : 1;        // NGTextEndEffect
};

DEFINE_TYPE_CASTS(NGPhysicalTextFragment,
                  NGPhysicalFragment,
                  fragment,
                  fragment->IsText(),
                  fragment.IsText());

}  // namespace blink

#endif  // NGPhysicalTextFragment_h
