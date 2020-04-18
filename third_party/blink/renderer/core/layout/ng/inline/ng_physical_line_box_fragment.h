// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGPhysicalLineBoxFragment_h
#define NGPhysicalLineBoxFragment_h

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_line_height_metrics.h"
#include "third_party/blink/renderer/core/layout/ng/ng_physical_container_fragment.h"
#include "third_party/blink/renderer/platform/fonts/font_baseline.h"

namespace blink {

class CORE_EXPORT NGPhysicalLineBoxFragment final
    : public NGPhysicalContainerFragment {
 public:
  // This modifies the passed-in children vector.
  NGPhysicalLineBoxFragment(const ComputedStyle&,
                            NGStyleVariant style_variant,
                            NGPhysicalSize size,
                            Vector<scoped_refptr<NGPhysicalFragment>>& children,
                            const NGPhysicalOffsetRect& contents_visual_rect,
                            const NGPhysicalOffsetRect& scrollable_overflow,
                            const NGLineHeightMetrics&,
                            TextDirection base_direction,
                            scoped_refptr<NGBreakToken> break_token = nullptr);

  const NGLineHeightMetrics& Metrics() const { return metrics_; }

  // The base direction of this line. Also known as the paragraph direction.
  // This may be different from the direction of the container box when
  // first-line style is used, or when 'unicode-bidi: plaintext' is used.
  TextDirection BaseDirection() const {
    return static_cast<TextDirection>(base_direction_);
  }

  // Compute baseline for the specified baseline type.
  LayoutUnit BaselinePosition(FontBaseline) const;

  // VisualRect of itself including contents, in the local coordinate.
  NGPhysicalOffsetRect VisualRectWithContents() const;

  // Scrollable overflow. including contents, in the local coordinate.
  NGPhysicalOffsetRect ScrollableOverflow() const {
    return scrollable_overflow_;
  }

  // Returns the first/last leaf fragment in the line in logical order. Returns
  // nullptr if the line box is empty.
  const NGPhysicalFragment* FirstLogicalLeaf() const;
  const NGPhysicalFragment* LastLogicalLeaf() const;
  // Returns the last leaf fragment in the line in logical order except line
  // break. Returns nullptr if such fragment doesn't exist.
  const NGPhysicalFragment* LastLogicalLeafIgnoringLineBreak() const;

  // Whether the content soft-wraps to the next line.
  bool HasSoftWrapToNextLine() const;

  scoped_refptr<NGPhysicalFragment> CloneWithoutOffset() const {
    Vector<scoped_refptr<NGPhysicalFragment>> children_copy(children_);
    return base::AdoptRef(new NGPhysicalLineBoxFragment(
        Style(), StyleVariant(), size_, children_copy, contents_visual_rect_,
        scrollable_overflow_, metrics_, BaseDirection(), break_token_));
  }

 private:
  NGPhysicalOffsetRect scrollable_overflow_;
  NGLineHeightMetrics metrics_;
};

DEFINE_TYPE_CASTS(NGPhysicalLineBoxFragment,
                  NGPhysicalFragment,
                  fragment,
                  fragment->Type() == NGPhysicalFragment::kFragmentLineBox,
                  fragment.Type() == NGPhysicalFragment::kFragmentLineBox);

}  // namespace blink

#endif  // NGPhysicalBoxFragment_h
