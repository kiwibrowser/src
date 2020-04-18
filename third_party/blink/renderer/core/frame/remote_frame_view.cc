// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/frame/remote_frame_view.h"

#include "third_party/blink/renderer/core/dom/element_visibility_observer.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/remote_frame.h"
#include "third_party/blink/renderer/core/frame/remote_frame_client.h"
#include "third_party/blink/renderer/core/html/html_frame_owner_element.h"
#include "third_party/blink/renderer/core/intersection_observer/intersection_observer_entry.h"
#include "third_party/blink/renderer/core/layout/layout_embedded_content.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/platform/geometry/int_rect.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "third_party/blink/renderer/platform/graphics/paint/cull_rect.h"
#include "third_party/blink/renderer/platform/graphics/paint/drawing_recorder.h"

namespace blink {

RemoteFrameView::RemoteFrameView(RemoteFrame* remote_frame)
    : remote_frame_(remote_frame), is_attached_(false) {
  DCHECK(remote_frame);
}

RemoteFrameView::~RemoteFrameView() = default;

LocalFrameView* RemoteFrameView::ParentFrameView() const {
  if (!is_attached_)
    return nullptr;

  Frame* parent_frame = remote_frame_->Tree().Parent();
  if (parent_frame && parent_frame->IsLocalFrame())
    return ToLocalFrame(parent_frame)->View();

  return nullptr;
}

void RemoteFrameView::AttachToLayout() {
  DCHECK(!is_attached_);
  is_attached_ = true;
  if (ParentFrameView()->IsVisible())
    SetParentVisible(true);

  SetupRenderThrottling();
  subtree_throttled_ = ParentFrameView()->CanThrottleRendering();

  FrameRectsChanged();
}

void RemoteFrameView::DetachFromLayout() {
  DCHECK(is_attached_);
  SetParentVisible(false);
  is_attached_ = false;
}

RemoteFrameView* RemoteFrameView::Create(RemoteFrame* remote_frame) {
  RemoteFrameView* view = new RemoteFrameView(remote_frame);
  view->Show();
  return view;
}

void RemoteFrameView::UpdateViewportIntersectionsForSubtree(
    DocumentLifecycle::LifecycleState target_state) {
  if (!remote_frame_->OwnerLayoutObject())
    return;
  if (target_state < DocumentLifecycle::kPaintClean)
    return;

  LocalFrameView* local_root_view =
      ToLocalFrame(remote_frame_->Tree().Parent())->LocalFrameRoot().View();
  if (!local_root_view)
    return;

  // Start with rect in remote frame's coordinate space. Then
  // mapToVisualRectInAncestorSpace will move it to the local root's coordinate
  // space and account for any clip from containing elements such as a
  // scrollable div. Passing nullptr as an argument to
  // mapToVisualRectInAncestorSpace causes it to be clipped to the viewport,
  // even if there are RemoteFrame ancestors in the frame tree.
  LayoutRect rect(0, 0, frame_rect_.Width(), frame_rect_.Height());
  rect.Move(remote_frame_->OwnerLayoutObject()->ContentBoxOffset());
  IntRect viewport_intersection;
  VisualRectFlags flags =
      RuntimeEnabledFeatures::IntersectionObserverGeometryMapperEnabled()
          ? kUseGeometryMapper
          : kDefaultVisualRectFlags;
  if (remote_frame_->OwnerLayoutObject()->MapToVisualRectInAncestorSpace(
          nullptr, rect, flags)) {
    IntRect root_visible_rect = local_root_view->VisibleContentRect();
    IntRect intersected_rect = EnclosingIntRect(rect);
    intersected_rect.Intersect(root_visible_rect);
    intersected_rect.Move(-local_root_view->ScrollOffsetInt());

    // Translate the intersection rect from the root frame's coordinate space
    // to the remote frame's coordinate space.
    FloatRect viewport_intersection_float =
        remote_frame_->OwnerLayoutObject()
            ->AncestorToLocalQuad(local_root_view->GetLayoutView(),
                                  FloatQuad(intersected_rect),
                                  kTraverseDocumentBoundaries | kUseTransforms)
            .BoundingBox();
    viewport_intersection_float.Move(
        -remote_frame_->OwnerLayoutObject()->ContentBoxOffset());
    viewport_intersection = EnclosingIntRect(viewport_intersection_float);
  }

  if (viewport_intersection == last_viewport_intersection_)
    return;

  last_viewport_intersection_ = viewport_intersection;
  remote_frame_->Client()->UpdateRemoteViewportIntersection(
      viewport_intersection);
}

IntRect RemoteFrameView::GetCompositingRect() {
  LocalFrameView* local_root_view =
      ToLocalFrame(remote_frame_->Tree().Parent())->LocalFrameRoot().View();
  if (!local_root_view || !remote_frame_->OwnerLayoutObject())
    return IntRect();

  // For main frames we constrain the rect that gets painted to the viewport.
  // If the local frame root is an OOPIF itself, then we use the root's
  // intersection rect. This represents a conservative maximum for the area
  // that needs to be rastered by the OOPIF compositor.
  IntSize viewport_size = local_root_view->FrameRect().Size();
  if (local_root_view->GetPage()->MainFrame() != local_root_view->GetFrame()) {
    viewport_size = local_root_view->RemoteViewportIntersection().Size();
  }

  // The viewport size needs to account for intermediate CSS transforms before
  // being compared to the frame size.
  FloatQuad viewport_quad =
      remote_frame_->OwnerLayoutObject()->AncestorToLocalQuad(
          local_root_view->GetLayoutView(),
          FloatRect(FloatPoint(), FloatSize(viewport_size)),
          kTraverseDocumentBoundaries | kUseTransforms);
  IntSize converted_viewport_size =
      EnclosingIntRect(viewport_quad.BoundingBox()).Size();

  IntSize frame_size = FrameRect().Size();

  // Iframes that fit within the window viewport get fully rastered. For
  // iframes that are larger than the window viewport, add a 30% buffer to the
  // draw area to try to prevent guttering during scroll.
  // TODO(kenrb): The 30% value is arbitrary, it gives 15% overdraw in both
  // directions when the iframe extends beyond both edges of the viewport, and
  // it seems to make guttering rare with slow to medium speed wheel scrolling.
  // Can we collect UMA data to estimate how much extra rastering this causes,
  // and possibly how common guttering is?
  converted_viewport_size.Scale(1.3f);
  converted_viewport_size.SetWidth(
      std::min(frame_size.Width(), converted_viewport_size.Width()));
  converted_viewport_size.SetHeight(
      std::min(frame_size.Height(), converted_viewport_size.Height()));
  IntPoint expanded_origin;
  if (!last_viewport_intersection_.IsEmpty()) {
    IntSize expanded_size =
        last_viewport_intersection_.Size().ExpandedTo(converted_viewport_size);
    expanded_size -= last_viewport_intersection_.Size();
    expanded_size.Scale(0.5f, 0.5f);
    expanded_origin = last_viewport_intersection_.Location() - expanded_size;
    expanded_origin.ClampNegativeToZero();
  }
  return IntRect(expanded_origin, converted_viewport_size);
}

void RemoteFrameView::Dispose() {
  HTMLFrameOwnerElement* owner_element = remote_frame_->DeprecatedLocalOwner();
  // ownerElement can be null during frame swaps, because the
  // RemoteFrameView is disconnected before detachment.
  if (owner_element && owner_element->OwnedEmbeddedContentView() == this)
    owner_element->SetEmbeddedContentView(nullptr);
}

void RemoteFrameView::InvalidateRect(const IntRect& rect) {
  auto* object = remote_frame_->OwnerLayoutObject();
  if (!object)
    return;

  LayoutRect repaint_rect(rect);
  repaint_rect.Move(object->BorderLeft() + object->PaddingLeft(),
                    object->BorderTop() + object->PaddingTop());
  object->InvalidatePaintRectangle(repaint_rect);
}

void RemoteFrameView::SetFrameRect(const IntRect& frame_rect) {
  if (frame_rect == frame_rect_)
    return;

  frame_rect_ = frame_rect;
  FrameRectsChanged();
}

IntRect RemoteFrameView::FrameRect() const {
  IntPoint location(frame_rect_.Location());

  // As an optimization, we don't include the root layer's scroll offset in the
  // frame rect.  As a result, we don't need to recalculate the frame rect every
  // time the root layer scrolls, but we need to add it in here.
  LayoutEmbeddedContent* owner = remote_frame_->OwnerLayoutObject();
  if (owner) {
    LayoutView* owner_layout_view = owner->View();
    DCHECK(owner_layout_view);
    if (owner_layout_view->HasOverflowClip())
      location.Move(-owner_layout_view->ScrolledContentOffset());
  }

  return IntRect(location, frame_rect_.Size());
}

void RemoteFrameView::FrameRectsChanged() {
  // Update the rect to reflect the position of the frame relative to the
  // containing local frame root. The position of the local root within
  // any remote frames, if any, is accounted for by the embedder.
  IntRect frame_rect(FrameRect());
  IntRect screen_space_rect = frame_rect;

  if (LocalFrameView* parent = ParentFrameView()) {
    screen_space_rect =
        parent->ConvertToRootFrame(parent->ContentsToFrame(screen_space_rect));
  }
  remote_frame_->Client()->FrameRectsChanged(frame_rect, screen_space_rect);
}

void RemoteFrameView::Paint(GraphicsContext& context,
                            const GlobalPaintFlags flags,
                            const CullRect& rect,
                            const IntSize& paint_offset) const {
  // Painting remote frames is only for printing.
  if (!context.Printing())
    return;

  if (!rect.IntersectsCullRect(FrameRect()))
    return;

  DrawingRecorder recorder(context, *GetFrame().OwnerLayoutObject(),
                           DisplayItem::kDocumentBackground);
  context.Save();
  context.Translate(paint_offset.Width(), paint_offset.Height());

  DCHECK(context.Canvas());
  // Inform the remote frame to print.
  uint32_t content_id = Print(FrameRect(), context.Canvas());

  // Record the place holder id on canvas.
  context.Canvas()->recordCustomData(content_id);
  context.Restore();
}

void RemoteFrameView::UpdateGeometry() {
  if (LayoutEmbeddedContent* layout = remote_frame_->OwnerLayoutObject())
    layout->UpdateGeometry(*this);
}

void RemoteFrameView::Hide() {
  self_visible_ = false;
  remote_frame_->Client()->VisibilityChanged(false);
}

void RemoteFrameView::Show() {
  self_visible_ = true;
  remote_frame_->Client()->VisibilityChanged(true);
}

void RemoteFrameView::SetParentVisible(bool visible) {
  if (parent_visible_ == visible)
    return;

  parent_visible_ = visible;
  if (!self_visible_)
    return;

  remote_frame_->Client()->VisibilityChanged(self_visible_ && parent_visible_);
}

void RemoteFrameView::SetupRenderThrottling() {
  if (visibility_observer_)
    return;

  Element* target_element = GetFrame().DeprecatedLocalOwner();
  if (!target_element)
    return;

  visibility_observer_ = new ElementVisibilityObserver(
      target_element, WTF::BindRepeating(
                          [](RemoteFrameView* remote_view, bool is_visible) {
                            remote_view->UpdateRenderThrottlingStatus(
                                !is_visible, remote_view->subtree_throttled_);
                          },
                          WrapWeakPersistent(this)));
  visibility_observer_->Start();
}

void RemoteFrameView::UpdateRenderThrottlingStatus(bool hidden,
                                                   bool subtree_throttled) {
  TRACE_EVENT0("blink", "RemoteFrameView::UpdateRenderThrottlingStatus");
  if (!remote_frame_->Client())
    return;

  bool was_throttled = CanThrottleRendering();

  // Note that we disallow throttling of 0x0 and display:none frames because
  // some sites use them to drive UI logic.
  HTMLFrameOwnerElement* owner_element = remote_frame_->DeprecatedLocalOwner();
  hidden_for_throttling_ = hidden && !frame_rect_.IsEmpty() &&
                           (owner_element && owner_element->GetLayoutObject());
  subtree_throttled_ = subtree_throttled;

  bool is_throttled = CanThrottleRendering();
  if (was_throttled != is_throttled) {
    remote_frame_->Client()->UpdateRenderThrottlingStatus(is_throttled,
                                                          subtree_throttled_);
  }
}

bool RemoteFrameView::CanThrottleRendering() const {
  if (!RuntimeEnabledFeatures::RenderingPipelineThrottlingEnabled())
    return false;
  if (subtree_throttled_)
    return true;
  return hidden_for_throttling_;
}

void RemoteFrameView::SetIntrinsicSizeInfo(
    const IntrinsicSizingInfo& size_info) {
  intrinsic_sizing_info_ = size_info;
  has_intrinsic_sizing_info_ = true;
}

bool RemoteFrameView::GetIntrinsicSizingInfo(
    IntrinsicSizingInfo& sizing_info) const {
  if (!has_intrinsic_sizing_info_)
    return false;

  sizing_info = intrinsic_sizing_info_;
  return true;
}

bool RemoteFrameView::HasIntrinsicSizingInfo() const {
  return has_intrinsic_sizing_info_;
}

uint32_t RemoteFrameView::Print(const IntRect& rect, WebCanvas* canvas) const {
  return remote_frame_->Client()->Print(rect, canvas);
}

void RemoteFrameView::Trace(blink::Visitor* visitor) {
  visitor->Trace(remote_frame_);
  visitor->Trace(visibility_observer_);
}

}  // namespace blink
