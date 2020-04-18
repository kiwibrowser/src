// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/layers/viewport.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "cc/input/browser_controls_offset_manager.h"
#include "cc/trees/layer_tree_host_impl.h"
#include "cc/trees/layer_tree_impl.h"
#include "cc/trees/scroll_node.h"
#include "ui/gfx/geometry/vector2d_conversions.h"
#include "ui/gfx/geometry/vector2d_f.h"

namespace cc {

// static
std::unique_ptr<Viewport> Viewport::Create(LayerTreeHostImpl* host_impl) {
  return base::WrapUnique(new Viewport(host_impl));
}

Viewport::Viewport(LayerTreeHostImpl* host_impl)
    : host_impl_(host_impl)
    , pinch_zoom_active_(false) {
  DCHECK(host_impl_);
}

void Viewport::Pan(const gfx::Vector2dF& delta) {
  gfx::Vector2dF pending_delta = delta;
  float page_scale = host_impl_->active_tree()->current_page_scale_factor();
  pending_delta.Scale(1 / page_scale);
  InnerScrollLayer()->ScrollBy(pending_delta);
}

Viewport::ScrollResult Viewport::ScrollBy(const gfx::Vector2dF& delta,
                                          const gfx::Point& viewport_point,
                                          bool is_direct_manipulation,
                                          bool affect_browser_controls,
                                          bool scroll_outer_viewport) {
  if (!OuterScrollLayer())
    return ScrollResult();

  gfx::Vector2dF content_delta = delta;

  if (affect_browser_controls && ShouldBrowserControlsConsumeScroll(delta))
    content_delta -= ScrollBrowserControls(delta);

  gfx::Vector2dF pending_content_delta = content_delta;

  ScrollTree& scroll_tree =
      host_impl_->active_tree()->property_trees()->scroll_tree;
  ScrollNode* inner_node =
      scroll_tree.Node(InnerScrollLayer()->scroll_tree_index());
  pending_content_delta -= host_impl_->ScrollSingleNode(
      inner_node, pending_content_delta, viewport_point, is_direct_manipulation,
      &scroll_tree);

  ScrollResult result;

  if (scroll_outer_viewport) {
    ScrollNode* outer_node =
        scroll_tree.Node(OuterScrollLayer()->scroll_tree_index());
    pending_content_delta -= host_impl_->ScrollSingleNode(
        outer_node, pending_content_delta, viewport_point,
        is_direct_manipulation, &scroll_tree);
  }

  result.consumed_delta = delta - AdjustOverscroll(pending_content_delta);

  result.content_scrolled_delta = content_delta - pending_content_delta;
  return result;
}

bool Viewport::CanScroll(const ScrollState& scroll_state) const {
  if (!OuterScrollLayer())
    return false;

  bool result = false;
  ScrollTree& scroll_tree =
      host_impl_->active_tree()->property_trees()->scroll_tree;
  ScrollNode* inner_node =
      scroll_tree.Node(InnerScrollLayer()->scroll_tree_index());
  if (inner_node)
    result |= host_impl_->CanConsumeDelta(*inner_node, scroll_state);
  ScrollNode* outer_node =
      scroll_tree.Node(OuterScrollLayer()->scroll_tree_index());
  if (outer_node)
    result |= host_impl_->CanConsumeDelta(*outer_node, scroll_state);

  return result;
}

void Viewport::ScrollByInnerFirst(const gfx::Vector2dF& delta) {
  LayerImpl* scroll_layer = InnerScrollLayer();

  gfx::Vector2dF unused_delta = scroll_layer->ScrollBy(delta);
  if (!unused_delta.IsZero() && OuterScrollLayer())
    OuterScrollLayer()->ScrollBy(unused_delta);
}

bool Viewport::ShouldAnimateViewport(const gfx::Vector2dF& viewport_delta,
                                     const gfx::Vector2dF& pending_delta) {
  float max_dim_viewport_delta =
      std::max(std::abs(viewport_delta.x()), std::abs(viewport_delta.y()));
  float max_dim_pending_delta =
      std::max(std::abs(pending_delta.x()), std::abs(pending_delta.y()));
  return max_dim_viewport_delta > max_dim_pending_delta;
}

gfx::Vector2dF Viewport::ScrollAnimated(const gfx::Vector2dF& delta,
                                        base::TimeDelta delayed_by) {
  if (!OuterScrollLayer())
    return gfx::Vector2dF(0, 0);

  ScrollTree& scroll_tree =
      host_impl_->active_tree()->property_trees()->scroll_tree;

  float scale_factor = host_impl_->active_tree()->current_page_scale_factor();
  gfx::Vector2dF scaled_delta = delta;
  scaled_delta.Scale(1.f / scale_factor);

  ScrollNode* inner_node =
      scroll_tree.Node(InnerScrollLayer()->scroll_tree_index());
  gfx::Vector2dF inner_delta =
      host_impl_->ComputeScrollDelta(*inner_node, delta);

  gfx::Vector2dF pending_delta = scaled_delta - inner_delta;
  pending_delta.Scale(scale_factor);

  ScrollNode* outer_node =
      scroll_tree.Node(OuterScrollLayer()->scroll_tree_index());
  gfx::Vector2dF outer_delta =
      host_impl_->ComputeScrollDelta(*outer_node, pending_delta);

  if (inner_delta.IsZero() && outer_delta.IsZero())
    return gfx::Vector2dF(0, 0);

  // Animate the viewport to which the majority of scroll delta will be applied.
  // The animation system only supports running one scroll offset animation.
  // TODO(ymalik): Fix the visible jump seen by instant scrolling one of the
  // viewports.
  bool will_animate = false;
  if (ShouldAnimateViewport(inner_delta, outer_delta)) {
    scroll_tree.ScrollBy(outer_node, outer_delta, host_impl_->active_tree());
    will_animate =
        host_impl_->ScrollAnimationCreate(inner_node, inner_delta, delayed_by);
  } else {
    scroll_tree.ScrollBy(inner_node, inner_delta, host_impl_->active_tree());
    will_animate =
        host_impl_->ScrollAnimationCreate(outer_node, outer_delta, delayed_by);
  }
  if (will_animate) {
    // Consume entire scroll delta as long as we are starting an animation.
    return delta;
  }

  pending_delta = scaled_delta - inner_delta - outer_delta;
  pending_delta.Scale(scale_factor);
  return pending_delta;
}

void Viewport::SnapPinchAnchorIfWithinMargin(const gfx::Point& anchor) {
  gfx::SizeF viewport_size = gfx::SizeF(
      host_impl_->active_tree()->InnerViewportContainerLayer()->bounds());

  if (anchor.x() < kPinchZoomSnapMarginDips)
    pinch_anchor_adjustment_.set_x(-anchor.x());
  else if (anchor.x() > viewport_size.width() - kPinchZoomSnapMarginDips)
    pinch_anchor_adjustment_.set_x(viewport_size.width() - anchor.x());

  if (anchor.y() < kPinchZoomSnapMarginDips)
    pinch_anchor_adjustment_.set_y(-anchor.y());
  else if (anchor.y() > viewport_size.height() - kPinchZoomSnapMarginDips)
    pinch_anchor_adjustment_.set_y(viewport_size.height() - anchor.y());
}

void Viewport::PinchUpdate(float magnify_delta, const gfx::Point& anchor) {
  if (!pinch_zoom_active_) {
    // If this is the first pinch update and the pinch is within a margin-
    // length of the screen edge, offset all updates by the amount so that we
    // effectively snap the pinch zoom to the edge of the screen. This makes it
    // easy to zoom in on position: fixed elements.
    SnapPinchAnchorIfWithinMargin(anchor);

    pinch_zoom_active_ = true;
  }

  LayerTreeImpl* active_tree = host_impl_->active_tree();

  // Keep the center-of-pinch anchor specified by (x, y) in a stable
  // position over the course of the magnify.
  gfx::Point adjusted_anchor = anchor + pinch_anchor_adjustment_;
  float page_scale = active_tree->current_page_scale_factor();
  gfx::PointF previous_scale_anchor =
      gfx::ScalePoint(gfx::PointF(adjusted_anchor), 1.f / page_scale);
  active_tree->SetPageScaleOnActiveTree(page_scale * magnify_delta);
  page_scale = active_tree->current_page_scale_factor();
  gfx::PointF new_scale_anchor =
      gfx::ScalePoint(gfx::PointF(adjusted_anchor), 1.f / page_scale);
  gfx::Vector2dF move = previous_scale_anchor - new_scale_anchor;

  // Scale back to viewport space since that's the coordinate space ScrollBy
  // uses.
  move.Scale(page_scale);

  // If clamping the inner viewport scroll offset causes a change, it should
  // be accounted for from the intended move.
  move -= InnerScrollLayer()->ClampScrollToMaxScrollOffset();

  Pan(move);
}

void Viewport::PinchEnd(const gfx::Point& anchor, bool snap_to_min) {
  LayerTreeImpl* active_tree = host_impl_->active_tree();
  // TODO(mcnee): Remove the InnerViewportScrollLayer() check (and add a
  // DCHECK instead), after we no longer send GesturePinch events to
  // OOPIF renderers. https://crbug.com/787924
  // Snap-to-min only makes sense for mainframes, so we use the lack of an
  // InnerViewportScrollLayer() to detect if this is an OOPIF.
  if (snap_to_min && active_tree->InnerViewportScrollLayer()) {
    const float kMaxZoomForSnapToMin = 1.05f;
    const base::TimeDelta kSnapToMinZoomAnimationDuration =
        base::TimeDelta::FromMilliseconds(200);
    float page_scale = active_tree->current_page_scale_factor();
    float min_scale = active_tree->min_page_scale_factor();

    // If the page is close to minimum scale at pinch end, snap to minimum.
    if (page_scale < min_scale * kMaxZoomForSnapToMin) {
      gfx::PointF adjusted_anchor =
          gfx::PointF(anchor + pinch_anchor_adjustment_);
      adjusted_anchor =
          gfx::ScalePoint(adjusted_anchor, min_scale / page_scale);
      adjusted_anchor += ScrollOffsetToVector2dF(TotalScrollOffset());
      host_impl_->StartPageScaleAnimation(
          ToRoundedVector2d(adjusted_anchor.OffsetFromOrigin()), true,
          min_scale, kSnapToMinZoomAnimationDuration);
    }
  }

  pinch_anchor_adjustment_ = gfx::Vector2d();
  pinch_zoom_active_ = false;
}

LayerImpl* Viewport::MainScrollLayer() const {
  return OuterScrollLayer();
}

gfx::Vector2dF Viewport::ScrollBrowserControls(const gfx::Vector2dF& delta) {
  gfx::Vector2dF excess_delta =
      host_impl_->browser_controls_manager()->ScrollBy(delta);

  return delta - excess_delta;
}

bool Viewport::ShouldBrowserControlsConsumeScroll(
    const gfx::Vector2dF& scroll_delta) const {
  // Always consume if it's in the direction to show the browser controls.
  if (scroll_delta.y() < 0)
    return true;

  if (TotalScrollOffset().y() < MaxTotalScrollOffset().y())
    return true;

  return false;
}

gfx::Vector2dF Viewport::AdjustOverscroll(const gfx::Vector2dF& delta) const {
  // TODO(tdresser): Use a more rational epsilon. See crbug.com/510550 for
  // details.
  const float kEpsilon = 0.1f;
  gfx::Vector2dF adjusted = delta;

  if (std::abs(adjusted.x()) < kEpsilon)
    adjusted.set_x(0.0f);
  if (std::abs(adjusted.y()) < kEpsilon)
    adjusted.set_y(0.0f);

  return adjusted;
}

gfx::ScrollOffset Viewport::MaxTotalScrollOffset() const {
  gfx::ScrollOffset offset;

  offset += InnerScrollLayer()->MaxScrollOffset();

  if (OuterScrollLayer())
    offset += OuterScrollLayer()->MaxScrollOffset();

  return offset;
}

gfx::ScrollOffset Viewport::TotalScrollOffset() const {
  gfx::ScrollOffset offset;

  offset += InnerScrollLayer()->CurrentScrollOffset();

  if (OuterScrollLayer())
    offset += OuterScrollLayer()->CurrentScrollOffset();

  return offset;
}

LayerImpl* Viewport::InnerScrollLayer() const {
  return host_impl_->InnerViewportScrollLayer();
}

LayerImpl* Viewport::OuterScrollLayer() const {
  return host_impl_->OuterViewportScrollLayer();
}

}  // namespace cc
