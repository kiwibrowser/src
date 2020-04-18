// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAGE_SCROLLING_STICKY_POSITION_SCROLLING_CONSTRAINTS_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAGE_SCROLLING_STICKY_POSITION_SCROLLING_CONSTRAINTS_H_

#include "third_party/blink/renderer/platform/geometry/float_rect.h"
#include "third_party/blink/renderer/platform/geometry/float_size.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"

namespace blink {

class PaintLayer;
class StickyPositionScrollingConstraints;

typedef WTF::HashMap<PaintLayer*, StickyPositionScrollingConstraints>
    StickyConstraintsMap;

// Encapsulates the constraint information for a position: sticky element and
// does calculation of the sticky offset for a given overflow clip rectangle.
//
// To avoid slowing down scrolling we cannot make the offset calculation a
// layout-inducing event. Instead constraint information is cached during layout
// and used as the scroll position changes to determine the current offset. In
// most cases the only information that is needed is the sticky element's layout
// rectangle and its containing block rectangle (both respective to the nearest
// ancestor scroller which the element is sticking to), and the set of sticky
// edge constraints (i.e. the distance from each edge the element should stick).
//
// For a given (non-cached) overflow clip rectangle, calculating the current
// offset in most cases just requires sliding the (cached) sticky element
// rectangle until it satisfies the (cached) sticky edge constraints for the
// overflow clip rectangle, whilst not letting the sticky element rectangle
// escape its (cached) containing block rect. For example, consider the
// following situation (where positions are relative to the scroll container):
//
//    +---------------------+ <-- Containing Block (150x70 at 10,0)
//    | +-----------------+ |
//    | |    top: 50px;   |<-- Sticky Box (130x10 at 20,0)
//    | +-----------------+ |
//    |                     |
//  +-------------------------+ <-- Overflow Clip Rectangle (170x60 at 0,50)
//  | |                     | |
//  | |                     | |
//  | +---------------------+ |
//  |                         |
//  |                         |
//  |                         |
//  +-------------------------+
//
// Here the cached sticky box would be moved downwards to try and be at position
// (20,100) - 50 pixels down from the top of the clip rectangle. However doing
// so would take it outside the cached containing block rectangle, so the final
// sticky position would be capped to (20,20).
//
// Unfortunately this approach breaks down in the presence of nested sticky
// elements, as the cached locations would be moved by ancestor sticky elements.
// Consider:
//
//  +------------------------+ <-- Outer sticky (top: 10px, 150x50 at 0,0)
//  |  +------------------+  |
//  |  |                  | <-- Inner sticky (top: 25px, 100x20 at 20,0)
//  |  +------------------+  |
//  |                        |
//  +------------------------+
//
// Assume the overflow clip rectangle is centered perfectly over the outer
// sticky. We would then want to move the outer sticky element down by 10
// pixels, and the inner sticky element down by only 15 pixels - because it is
// already being shifted by its ancestor. To correctly handle such situations we
// apply more complicated logic which is explained in the implementation of
// |ComputeStickyOffset|.
class StickyPositionScrollingConstraints final {
 public:
  enum AnchorEdgeFlags {
    kAnchorEdgeLeft = 1 << 0,
    kAnchorEdgeRight = 1 << 1,
    kAnchorEdgeTop = 1 << 2,
    kAnchorEdgeBottom = 1 << 3
  };
  typedef unsigned AnchorEdges;

  StickyPositionScrollingConstraints()
      : anchor_edges_(0),
        left_offset_(0),
        right_offset_(0),
        top_offset_(0),
        bottom_offset_(0),
        nearest_sticky_layer_shifting_sticky_box_(nullptr),
        nearest_sticky_layer_shifting_containing_block_(nullptr) {}

  StickyPositionScrollingConstraints(
      const StickyPositionScrollingConstraints& other) = default;

  // Computes the sticky offset for a given overflow clip rect.
  //
  // This method is non-const as we cache internal state for performance; see
  // documentation in the implementation for details.
  FloatSize ComputeStickyOffset(const FloatRect& overflow_clip_rect,
                                const StickyConstraintsMap&);

  // Returns the last-computed offset of the sticky box from its original
  // position before scroll.
  //
  // This method exists for performance (to avoid recomputing the sticky offset)
  // and must only be called when compositing inputs are clean for the sticky
  // element. (Or after prepaint for SlimmingPaintV2).
  FloatSize GetOffsetForStickyPosition(const StickyConstraintsMap&) const;

  bool HasAncestorStickyElement() const {
    return nearest_sticky_layer_shifting_sticky_box_ ||
           nearest_sticky_layer_shifting_containing_block_;
  }

  AnchorEdges GetAnchorEdges() const { return anchor_edges_; }
  bool HasAnchorEdge(AnchorEdgeFlags flag) const {
    return anchor_edges_ & flag;
  }
  void AddAnchorEdge(AnchorEdgeFlags edge_flag) { anchor_edges_ |= edge_flag; }

  float LeftOffset() const { return left_offset_; }
  float RightOffset() const { return right_offset_; }
  float TopOffset() const { return top_offset_; }
  float BottomOffset() const { return bottom_offset_; }

  void SetLeftOffset(float offset) { left_offset_ = offset; }
  void SetRightOffset(float offset) { right_offset_ = offset; }
  void SetTopOffset(float offset) { top_offset_ = offset; }
  void SetBottomOffset(float offset) { bottom_offset_ = offset; }

  void SetScrollContainerRelativeContainingBlockRect(const FloatRect& rect) {
    scroll_container_relative_containing_block_rect_ = rect;
  }
  const FloatRect& ScrollContainerRelativeContainingBlockRect() const {
    return scroll_container_relative_containing_block_rect_;
  }

  void SetScrollContainerRelativeStickyBoxRect(const FloatRect& rect) {
    scroll_container_relative_sticky_box_rect_ = rect;
  }
  const FloatRect& ScrollContainerRelativeStickyBoxRect() const {
    return scroll_container_relative_sticky_box_rect_;
  }

  void SetNearestStickyLayerShiftingStickyBox(PaintLayer* layer) {
    nearest_sticky_layer_shifting_sticky_box_ = layer;
  }
  PaintLayer* NearestStickyLayerShiftingStickyBox() const {
    return nearest_sticky_layer_shifting_sticky_box_;
  }

  void SetNearestStickyLayerShiftingContainingBlock(PaintLayer* layer) {
    nearest_sticky_layer_shifting_containing_block_ = layer;
  }
  PaintLayer* NearestStickyLayerShiftingContainingBlock() const {
    return nearest_sticky_layer_shifting_containing_block_;
  }

  bool operator==(const StickyPositionScrollingConstraints& other) const {
    return left_offset_ == other.left_offset_ &&
           right_offset_ == other.right_offset_ &&
           top_offset_ == other.top_offset_ &&
           bottom_offset_ == other.bottom_offset_ &&
           scroll_container_relative_containing_block_rect_ ==
               other.scroll_container_relative_containing_block_rect_ &&
           scroll_container_relative_sticky_box_rect_ ==
               other.scroll_container_relative_sticky_box_rect_ &&
           nearest_sticky_layer_shifting_sticky_box_ ==
               other.nearest_sticky_layer_shifting_sticky_box_ &&
           nearest_sticky_layer_shifting_containing_block_ ==
               other.nearest_sticky_layer_shifting_containing_block_ &&
           total_sticky_box_sticky_offset_ ==
               other.total_sticky_box_sticky_offset_ &&
           total_containing_block_sticky_offset_ ==
               other.total_containing_block_sticky_offset_;
  }

  bool operator!=(const StickyPositionScrollingConstraints& other) const {
    return !(*this == other);
  }

 private:
  FloatSize AncestorStickyBoxOffset(const StickyConstraintsMap&);
  FloatSize AncestorContainingBlockOffset(const StickyConstraintsMap&);

  AnchorEdges anchor_edges_;
  float left_offset_;
  float right_offset_;
  float top_offset_;
  float bottom_offset_;

  // The containing block rect and sticky box rect are the basic components
  // for calculating the sticky offset to apply after a scroll. Consider the
  // following setup:
  //
  // <scroll-container>
  //   <containing-block> (*)
  //     <sticky-element>
  //
  // (*) <containing-block> may be the same as <scroll-container>.

  // The layout position of the containing block relative to the scroll
  // container. When calculating the sticky offset it is used to ensure the
  // sticky element stays bounded by its containing block.
  FloatRect scroll_container_relative_containing_block_rect_;

  // The layout position of the sticky element relative to the scroll container.
  // When calculating the sticky offset it is used to determine how large the
  // offset needs to be to satisfy the sticky constraints.
  FloatRect scroll_container_relative_sticky_box_rect_;

  // In the case of nested sticky elements the layout position of the sticky
  // element and its containing block are not accurate (as they are affected by
  // ancestor sticky offsets). To ensure a correct sticky offset calculation in
  // that case we must track any sticky ancestors between the sticky element and
  // its containing block, and between its containing block and the overflow
  // clip ancestor.
  //
  // See the implementation of |ComputeStickyOffset| for documentation on how
  // these ancestors are used to correct the offset calculation.
  PaintLayer* nearest_sticky_layer_shifting_sticky_box_;
  PaintLayer* nearest_sticky_layer_shifting_containing_block_;

  // For performance we cache our accumulated sticky offset to allow descendant
  // sticky elements to offset their constraint rects. Because we can either
  // affect a descendant element's sticky box constraint rect or containing
  // block constraint rect, we need to accumulate two offsets.

  // The sticky box offset accumulates the chain of sticky elements that are
  // between this sticky element and its containing block. Any descendant using
  // |total_sticky_box_sticky_offset_| has the same containing block as this
  // element, so |total_sticky_box_sticky_offset_| does not accumulate
  // containing block sticky offsets. For example, consider the following chain:
  //
  // <div style="position: sticky;">
  //   <div id="outerInline" style="position: sticky; display: inline;">
  //     <div id="innerInline" style="position: sticky; display: inline;"><div>
  //   </div>
  // </div>
  //
  // In the above example, both outerInline and innerInline have the same
  // containing block - the outermost <div>.
  FloatSize total_sticky_box_sticky_offset_;

  // The containing block offset accumulates all sticky-related offsets between
  // this element and the ancestor scroller. If this element is a containing
  // block shifting ancestor for some descendant, it shifts the descendant's
  // constraint rects by its entire offset.
  FloatSize total_containing_block_sticky_offset_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAGE_SCROLLING_STICKY_POSITION_SCROLLING_CONSTRAINTS_H_
