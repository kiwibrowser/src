// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/page/scrolling/sticky_position_scrolling_constraints.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"

namespace blink {

FloatSize StickyPositionScrollingConstraints::ComputeStickyOffset(
    const FloatRect& overflow_clip_rect,
    const StickyConstraintsMap& constraints_map) {
  FloatRect sticky_box_rect = scroll_container_relative_sticky_box_rect_;
  FloatRect containing_block_rect =
      scroll_container_relative_containing_block_rect_;
  FloatSize ancestor_sticky_box_offset =
      AncestorStickyBoxOffset(constraints_map);
  FloatSize ancestor_containing_block_offset =
      AncestorContainingBlockOffset(constraints_map);

  // Adjust the cached rect locations for any sticky ancestor elements. The
  // sticky offset applied to those ancestors affects us as follows:
  //
  //   1. |nearest_sticky_layer_shifting_sticky_box_| is a sticky layer between
  //      ourselves and our containing block, e.g. a nested inline parent.
  //      It shifts only the sticky_box_rect and not the containing_block_rect.
  //   2. |nearest_sticky_layer_shifting_containing_block_| is a sticky layer
  //      between our containing block (inclusive) and our scroll ancestor
  //      (exclusive). As such, it shifts both the sticky_box_rect and the
  //      containing_block_rect.
  //
  // Note that this calculation assumes that |ComputeStickyOffset| is being
  // called top down, e.g. it has been called on any ancestors we have before
  // being called on us.
  sticky_box_rect.Move(ancestor_sticky_box_offset +
                       ancestor_containing_block_offset);
  containing_block_rect.Move(ancestor_containing_block_offset);

  // We now attempt to shift sticky_box_rect to obey the specified sticky
  // constraints, whilst always staying within our containing block. This
  // shifting produces the final sticky offset below.
  //
  // As per the spec, 'left' overrides 'right' and 'top' overrides 'bottom'.
  FloatRect box_rect = sticky_box_rect;

  if (HasAnchorEdge(kAnchorEdgeRight)) {
    float right_limit = overflow_clip_rect.MaxX() - right_offset_;
    float right_delta =
        std::min<float>(0, right_limit - sticky_box_rect.MaxX());
    float available_space =
        std::min<float>(0, containing_block_rect.X() - sticky_box_rect.X());
    if (right_delta < available_space)
      right_delta = available_space;

    box_rect.Move(right_delta, 0);
  }

  if (HasAnchorEdge(kAnchorEdgeLeft)) {
    float left_limit = overflow_clip_rect.X() + left_offset_;
    float left_delta = std::max<float>(0, left_limit - sticky_box_rect.X());
    float available_space = std::max<float>(
        0, containing_block_rect.MaxX() - sticky_box_rect.MaxX());
    if (left_delta > available_space)
      left_delta = available_space;

    box_rect.Move(left_delta, 0);
  }

  if (HasAnchorEdge(kAnchorEdgeBottom)) {
    float bottom_limit = overflow_clip_rect.MaxY() - bottom_offset_;
    float bottom_delta =
        std::min<float>(0, bottom_limit - sticky_box_rect.MaxY());
    float available_space =
        std::min<float>(0, containing_block_rect.Y() - sticky_box_rect.Y());
    if (bottom_delta < available_space)
      bottom_delta = available_space;

    box_rect.Move(0, bottom_delta);
  }

  if (HasAnchorEdge(kAnchorEdgeTop)) {
    float top_limit = overflow_clip_rect.Y() + top_offset_;
    float top_delta = std::max<float>(0, top_limit - sticky_box_rect.Y());
    float available_space = std::max<float>(
        0, containing_block_rect.MaxY() - sticky_box_rect.MaxY());
    if (top_delta > available_space)
      top_delta = available_space;

    box_rect.Move(0, top_delta);
  }

  FloatSize sticky_offset = box_rect.Location() - sticky_box_rect.Location();

  // Now that we have computed our current sticky offset, update the cached
  // accumulated sticky offsets.
  total_sticky_box_sticky_offset_ = ancestor_sticky_box_offset + sticky_offset;
  total_containing_block_sticky_offset_ = ancestor_sticky_box_offset +
                                          ancestor_containing_block_offset +
                                          sticky_offset;

  return sticky_offset;
}

FloatSize StickyPositionScrollingConstraints::GetOffsetForStickyPosition(
    const StickyConstraintsMap& constraints_map) const {
  FloatSize nearest_sticky_layer_shifting_sticky_box_constraints_offset;
  if (nearest_sticky_layer_shifting_sticky_box_) {
    nearest_sticky_layer_shifting_sticky_box_constraints_offset =
        constraints_map.at(nearest_sticky_layer_shifting_sticky_box_)
            .total_sticky_box_sticky_offset_;
  }
  return total_sticky_box_sticky_offset_ -
         nearest_sticky_layer_shifting_sticky_box_constraints_offset;
}

FloatSize StickyPositionScrollingConstraints::AncestorStickyBoxOffset(
    const StickyConstraintsMap& constraints_map) {
  if (!nearest_sticky_layer_shifting_sticky_box_)
    return FloatSize();
  DCHECK(constraints_map.Contains(nearest_sticky_layer_shifting_sticky_box_));
  return constraints_map.at(nearest_sticky_layer_shifting_sticky_box_)
      .total_sticky_box_sticky_offset_;
}

FloatSize StickyPositionScrollingConstraints::AncestorContainingBlockOffset(
    const StickyConstraintsMap& constraints_map) {
  if (!nearest_sticky_layer_shifting_containing_block_) {
    return FloatSize();
  }
  DCHECK(constraints_map.Contains(
      nearest_sticky_layer_shifting_containing_block_));
  return constraints_map.at(nearest_sticky_layer_shifting_containing_block_)
      .total_containing_block_sticky_offset_;
}

}  // namespace blink
