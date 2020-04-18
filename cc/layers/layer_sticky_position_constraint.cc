// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/layers/layer_sticky_position_constraint.h"

#include "cc/layers/layer.h"

namespace cc {

LayerStickyPositionConstraint::LayerStickyPositionConstraint()
    : is_sticky(false),
      is_anchored_left(false),
      is_anchored_right(false),
      is_anchored_top(false),
      is_anchored_bottom(false),
      left_offset(0.f),
      right_offset(0.f),
      top_offset(0.f),
      bottom_offset(0.f),
      nearest_element_shifting_sticky_box(kInvalidElementId),
      nearest_element_shifting_containing_block(kInvalidElementId) {}

LayerStickyPositionConstraint::LayerStickyPositionConstraint(
    const LayerStickyPositionConstraint& other) = default;

bool LayerStickyPositionConstraint::operator==(
    const LayerStickyPositionConstraint& other) const {
  if (!is_sticky && !other.is_sticky)
    return true;
  return is_sticky == other.is_sticky &&
         is_anchored_left == other.is_anchored_left &&
         is_anchored_right == other.is_anchored_right &&
         is_anchored_top == other.is_anchored_top &&
         is_anchored_bottom == other.is_anchored_bottom &&
         left_offset == other.left_offset &&
         right_offset == other.right_offset && top_offset == other.top_offset &&
         bottom_offset == other.bottom_offset &&
         scroll_container_relative_sticky_box_rect ==
             other.scroll_container_relative_sticky_box_rect &&
         scroll_container_relative_containing_block_rect ==
             other.scroll_container_relative_containing_block_rect &&
         nearest_element_shifting_sticky_box ==
             other.nearest_element_shifting_sticky_box &&
         nearest_element_shifting_containing_block ==
             other.nearest_element_shifting_containing_block;
}

bool LayerStickyPositionConstraint::operator!=(
    const LayerStickyPositionConstraint& other) const {
  return !(*this == other);
}

ElementId LayerStickyPositionConstraint::NearestStickyAncestor() {
  return nearest_element_shifting_sticky_box
             ? nearest_element_shifting_sticky_box
             : nearest_element_shifting_containing_block;
}

}  // namespace cc
