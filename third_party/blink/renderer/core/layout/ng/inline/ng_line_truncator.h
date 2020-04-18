// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_NG_INLINE_NG_LINE_TRUNCATOR_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_NG_INLINE_NG_LINE_TRUNCATOR_H_

#include "base/optional.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_line_box_fragment_builder.h"
#include "third_party/blink/renderer/platform/text/text_direction.h"

namespace blink {

class NGLineInfo;

// A class to truncate lines and place ellipsis, invoked by the CSS
// 'text-overflow: ellipsis' property.
// https://drafts.csswg.org/css-ui/#overflow-ellipsis
class CORE_EXPORT NGLineTruncator final {
  STACK_ALLOCATED();

 public:
  NGLineTruncator(NGInlineNode& node, const NGLineInfo& line_info);

  // Truncate |line_box| and place ellipsis. Returns the new inline-size of the
  // |line_box|.
  //
  // |line_box| should be after bidi reorder, but before box fragments are
  // created.
  LayoutUnit TruncateLine(LayoutUnit line_width,
                          NGLineBoxFragmentBuilder::ChildList* line_box);

 private:
  base::Optional<LayoutUnit> EllipsisOffset(LayoutUnit line_width,
                                            LayoutUnit ellipsis_width,
                                            bool is_first_child,
                                            NGLineBoxFragmentBuilder::Child*);
  bool TruncateChild(LayoutUnit space_for_this_child,
                     bool is_first_child,
                     NGLineBoxFragmentBuilder::Child* child);

  NGInlineNode& node_;
  scoped_refptr<const ComputedStyle> line_style_;
  LayoutUnit available_width_;
  TextDirection line_direction_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_NG_INLINE_NG_LINE_TRUNCATOR_H_
