// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/geometry/ng_static_position.h"

namespace blink {

NGStaticPosition NGStaticPosition::Create(WritingMode writing_mode,
                                          TextDirection direction,
                                          NGPhysicalOffset offset) {
  NGStaticPosition position;
  position.offset = offset;
  switch (writing_mode) {
    case WritingMode::kHorizontalTb:
      position.type = (direction == TextDirection::kLtr) ? kTopLeft : kTopRight;
      break;
    case WritingMode::kVerticalRl:
    case WritingMode::kSidewaysRl:
      position.type =
          (direction == TextDirection::kLtr) ? kTopRight : kBottomRight;
      break;
    case WritingMode::kVerticalLr:
      position.type =
          (direction == TextDirection::kLtr) ? kTopLeft : kBottomLeft;
      break;
    case WritingMode::kSidewaysLr:
      position.type =
          (direction == TextDirection::kLtr) ? kBottomLeft : kTopLeft;
      break;
  }
  return position;
}

LayoutUnit NGStaticPosition::LeftInset(LayoutUnit container_size,
                                       LayoutUnit width,
                                       LayoutUnit margin_left,
                                       LayoutUnit margin_right) const {
  if (HasLeft())
    return offset.left;
  else
    return offset.left - width - margin_left - margin_right;
}

LayoutUnit NGStaticPosition::RightInset(LayoutUnit container_size,
                                        LayoutUnit width,
                                        LayoutUnit margin_left,
                                        LayoutUnit margin_right) const {
  if (HasLeft())
    return container_size - offset.left - width - margin_left - margin_right;
  else
    return container_size - offset.left;
}

LayoutUnit NGStaticPosition::TopInset(LayoutUnit container_size,
                                      LayoutUnit height,
                                      LayoutUnit margin_top,
                                      LayoutUnit margin_bottom) const {
  if (HasTop())
    return offset.top;
  else
    return offset.top - height - margin_bottom - margin_top;
}

LayoutUnit NGStaticPosition::BottomInset(LayoutUnit container_size,
                                         LayoutUnit height,
                                         LayoutUnit margin_top,
                                         LayoutUnit margin_bottom) const {
  if (HasTop())
    return container_size - offset.top - height - margin_top - margin_bottom;
  else
    return container_size - offset.top;
}

LayoutUnit NGStaticPosition::Left() const {
  DCHECK(HasLeft());
  return offset.left;
}

LayoutUnit NGStaticPosition::Right() const {
  DCHECK(!HasLeft());
  return offset.left;
}

LayoutUnit NGStaticPosition::Top() const {
  DCHECK(HasTop());
  return offset.top;
}

LayoutUnit NGStaticPosition::Bottom() const {
  DCHECK(!HasTop());
  return offset.top;
}

}  // namespace blink
