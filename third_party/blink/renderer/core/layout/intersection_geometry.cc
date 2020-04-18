// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/intersection_geometry.h"

#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/html/html_frame_owner_element.h"
#include "third_party/blink/renderer/core/layout/layout_box.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"

namespace blink {

namespace {

bool IsContainingBlockChainDescendant(LayoutObject* descendant,
                                      LayoutObject* ancestor) {
  LocalFrame* ancestor_frame = ancestor->GetDocument().GetFrame();
  LocalFrame* descendant_frame = descendant->GetDocument().GetFrame();
  if (ancestor_frame != descendant_frame)
    return false;

  while (descendant && descendant != ancestor)
    descendant = descendant->ContainingBlock();
  return descendant;
}

void MapRectUpToDocument(LayoutRect& rect,
                         const LayoutObject& descendant,
                         const Document& document) {
  FloatQuad mapped_quad = descendant.LocalToAncestorQuad(
      FloatQuad(FloatRect(rect)), document.GetLayoutView(),
      kUseTransforms | kApplyContainerFlip);
  rect = LayoutRect(mapped_quad.BoundingBox());
}

void MapRectDownToDocument(LayoutRect& rect,
                           LayoutBoxModelObject* ancestor,
                           const Document& document) {
  FloatQuad mapped_quad = document.GetLayoutView()->AncestorToLocalQuad(
      ancestor, FloatQuad(FloatRect(rect)),
      kUseTransforms | kApplyContainerFlip | kTraverseDocumentBoundaries);
  rect = LayoutRect(mapped_quad.BoundingBox());
}

LayoutUnit ComputeMargin(const Length& length, LayoutUnit reference_length) {
  if (length.GetType() == kPercent) {
    return LayoutUnit(static_cast<int>(reference_length.ToFloat() *
                                       length.Percent() / 100.0));
  }
  DCHECK_EQ(length.GetType(), kFixed);
  return LayoutUnit(length.IntValue());
}

LayoutView* LocalRootView(Element& element) {
  LocalFrame* frame = element.GetDocument().GetFrame();
  LocalFrame* frame_root = frame ? &frame->LocalFrameRoot() : nullptr;
  return frame_root ? frame_root->ContentLayoutObject() : nullptr;
}

}  // namespace

IntersectionGeometry::IntersectionGeometry(Element* root,
                                           Element& target,
                                           const Vector<Length>& root_margin,
                                           bool should_report_root_bounds)
    : root_(root ? root->GetLayoutObject() : LocalRootView(target)),
      target_(target.GetLayoutObject()),
      root_margin_(root_margin),
      does_intersect_(0),
      should_report_root_bounds_(should_report_root_bounds),
      root_is_implicit_(!root),
      can_compute_geometry_(InitializeCanComputeGeometry(root, target)) {
  if (can_compute_geometry_)
    InitializeGeometry();
}

IntersectionGeometry::~IntersectionGeometry() = default;

bool IntersectionGeometry::InitializeCanComputeGeometry(Element* root,
                                                        Element& target) const {
  DCHECK(root_margin_.IsEmpty() || root_margin_.size() == 4);
  if (root && !root->isConnected())
    return false;
  if (!root_ || !root_->IsBox())
    return false;
  if (!target.isConnected())
    return false;
  if (!target_ || (!target_->IsBoxModelObject() && !target_->IsText()))
    return false;
  if (root && !IsContainingBlockChainDescendant(target_, root_))
    return false;
  return true;
}

void IntersectionGeometry::InitializeGeometry() {
  InitializeTargetRect();
  intersection_rect_ = target_rect_;
  InitializeRootRect();
}

void IntersectionGeometry::InitializeTargetRect() {
  if (target_->IsBoxModelObject()) {
    target_rect_ =
        LayoutRect(ToLayoutBoxModelObject(target_)->BorderBoundingBox());
  } else {
    target_rect_ = ToLayoutText(target_)->LinesBoundingBox();
  }
}

void IntersectionGeometry::InitializeRootRect() {
  if (root_->IsLayoutView() && root_->GetDocument().IsInMainFrame()) {
    // The main frame is a bit special as the scrolling viewport can differ in
    // size from the LayoutView itself. There's two situations this occurs in:
    // 1) The ForceZeroLayoutHeight quirk setting is used in Android WebView for
    // compatibility and sets the initial-containing-block's (a.k.a.
    // LayoutView) height to 0. Thus, we can't use its size for intersection
    // testing. Use the FrameView geometry instead.
    // 2) An element wider than the ICB can cause us to resize the FrameView so
    // we can zoom out to fit the entire element width.
    root_rect_ = ToLayoutView(root_)->OverflowClipRect(LayoutPoint());
  } else if (root_->IsBox() && root_->HasOverflowClip()) {
    root_rect_ = LayoutRect(ToLayoutBox(root_)->ContentBoxRect());
  } else {
    root_rect_ = LayoutRect(ToLayoutBoxModelObject(root_)->BorderBoundingBox());
  }
  ApplyRootMargin();
}

void IntersectionGeometry::ApplyRootMargin() {
  if (root_margin_.IsEmpty())
    return;

  // TODO(szager): Make sure the spec is clear that left/right margins are
  // resolved against width and not height.
  LayoutUnit top_margin = ComputeMargin(root_margin_[0], root_rect_.Height());
  LayoutUnit right_margin = ComputeMargin(root_margin_[1], root_rect_.Width());
  LayoutUnit bottom_margin =
      ComputeMargin(root_margin_[2], root_rect_.Height());
  LayoutUnit left_margin = ComputeMargin(root_margin_[3], root_rect_.Width());

  root_rect_.SetX(root_rect_.X() - left_margin);
  root_rect_.SetWidth(root_rect_.Width() + left_margin + right_margin);
  root_rect_.SetY(root_rect_.Y() - top_margin);
  root_rect_.SetHeight(root_rect_.Height() + top_margin + bottom_margin);
}

void IntersectionGeometry::ClipToRoot() {
  // Map and clip rect into root element coordinates.
  // TODO(szager): the writing mode flipping needs a test.
  LayoutBox* local_ancestor = nullptr;
  if (!RootIsImplicit() || root_->GetDocument().IsInMainFrame())
    local_ancestor = ToLayoutBox(root_);
  VisualRectFlags flags = static_cast<VisualRectFlags>(
      RuntimeEnabledFeatures::IntersectionObserverGeometryMapperEnabled()
          ? (kUseGeometryMapper | kEdgeInclusive)
          : kEdgeInclusive);
  does_intersect_ = target_->MapToVisualRectInAncestorSpace(
      local_ancestor, intersection_rect_, flags);
  if (!does_intersect_ || !local_ancestor)
    return;
  if (local_ancestor->HasOverflowClip())
    intersection_rect_.Move(-local_ancestor->ScrolledContentOffset());
  LayoutRect root_clip_rect(root_rect_);
  local_ancestor->FlipForWritingMode(root_clip_rect);
  does_intersect_ &= intersection_rect_.InclusiveIntersect(root_clip_rect);
}

void IntersectionGeometry::MapTargetRectToTargetFrameCoordinates() {
  Document& target_document = target_->GetDocument();
  LayoutSize scroll_position =
      LayoutSize(target_document.View()->GetScrollOffset());
  MapRectUpToDocument(target_rect_, *target_, target_document);
  target_rect_.Move(-scroll_position);
}

void IntersectionGeometry::MapRootRectToRootFrameCoordinates() {
  root_->GetFrameView()->MapQuadToAncestorFrameIncludingScrollOffset(
      root_rect_, root_,
      RootIsImplicit() ? nullptr : root_->GetDocument().GetLayoutView(),
      kUseTransforms | kApplyContainerFlip);
}

void IntersectionGeometry::MapIntersectionRectToTargetFrameCoordinates() {
  Document& target_document = target_->GetDocument();
  if (RootIsImplicit()) {
    LocalFrame* target_frame = target_document.GetFrame();
    Frame& root_frame = target_frame->Tree().Top();
    LayoutSize scroll_position =
        LayoutSize(target_document.View()->GetScrollOffset());
    if (target_frame != &root_frame)
      MapRectDownToDocument(intersection_rect_, nullptr, target_document);
    intersection_rect_.Move(-scroll_position);
  } else {
    LayoutSize scroll_position =
        LayoutSize(target_document.View()->GetScrollOffset());
    MapRectUpToDocument(intersection_rect_, *root_, root_->GetDocument());
    intersection_rect_.Move(-scroll_position);
  }
}

void IntersectionGeometry::ComputeGeometry() {
  if (!CanComputeGeometry())
    return;
  DCHECK(root_);
  DCHECK(target_);
  ClipToRoot();
  MapTargetRectToTargetFrameCoordinates();
  if (does_intersect_)
    MapIntersectionRectToTargetFrameCoordinates();
  else
    intersection_rect_ = LayoutRect();
  // Small optimization: if we're not going to report root bounds, don't bother
  // transforming them to the frame.
  if (ShouldReportRootBounds())
    MapRootRectToRootFrameCoordinates();
}

}  // namespace blink
