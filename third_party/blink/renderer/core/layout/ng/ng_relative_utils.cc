// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/ng_relative_utils.h"

#include "base/optional.h"
#include "third_party/blink/renderer/core/layout/ng/geometry/ng_logical_offset.h"
#include "third_party/blink/renderer/core/layout/ng/geometry/ng_physical_size.h"
#include "third_party/blink/renderer/core/style/computed_style.h"
#include "third_party/blink/renderer/platform/length_functions.h"

namespace blink {

// Returns the child's relative position wrt the containing fragment.
NGLogicalOffset ComputeRelativeOffset(const ComputedStyle& child_style,
                                      WritingMode container_writing_mode,
                                      TextDirection container_direction,
                                      NGLogicalSize container_logical_size) {
  NGLogicalOffset offset;
  NGPhysicalSize container_size =
      container_logical_size.ConvertToPhysical(container_writing_mode);

  base::Optional<LayoutUnit> left, right, top, bottom;

  if (!child_style.Left().IsAuto())
    left = ValueForLength(child_style.Left(), container_size.width);
  if (!child_style.Right().IsAuto())
    right = ValueForLength(child_style.Right(), container_size.width);
  if (!child_style.Top().IsAuto())
    top = ValueForLength(child_style.Top(), container_size.height);
  if (!child_style.Bottom().IsAuto())
    bottom = ValueForLength(child_style.Bottom(), container_size.height);

  // Implements confict resolution rules from spec:
  // https://www.w3.org/TR/css-position-3/#rel-pos
  if (!left && !right) {
    left = LayoutUnit();
    right = LayoutUnit();
  }
  if (!left)
    left = -right.value();
  if (!right)
    right = -left.value();
  if (!top && !bottom) {
    top = LayoutUnit();
    bottom = LayoutUnit();
  }
  if (!top)
    top = -bottom.value();
  if (!bottom)
    bottom = -top.value();

  bool is_ltr = container_direction == TextDirection::kLtr;

  switch (container_writing_mode) {
    case WritingMode::kHorizontalTb:
      offset.inline_offset = is_ltr ? left.value() : right.value();
      offset.block_offset = top.value();
      break;
    case WritingMode::kVerticalRl:
    case WritingMode::kSidewaysRl:
      offset.inline_offset = is_ltr ? top.value() : bottom.value();
      offset.block_offset = right.value();
      break;
    case WritingMode::kVerticalLr:
      offset.inline_offset = is_ltr ? top.value() : bottom.value();
      offset.block_offset = left.value();
      break;
    case WritingMode::kSidewaysLr:
      offset.inline_offset = is_ltr ? bottom.value() : top.value();
      offset.block_offset = left.value();
      break;
  }
  return offset;
}

}  // namespace blink
