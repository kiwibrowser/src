// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/page/scrolling/root_scroller_controller.h"

#include "third_party/blink/renderer/core/css/style_change_reason.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/frame/browser_controls.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/fullscreen/document_fullscreen.h"
#include "third_party/blink/renderer/core/html/html_frame_owner_element.h"
#include "third_party/blink/renderer/core/layout/layout_box.h"
#include "third_party/blink/renderer/core/layout/layout_embedded_content.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/page/chrome_client.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/page/scrolling/root_scroller_util.h"
#include "third_party/blink/renderer/core/page/scrolling/top_document_root_scroller_controller.h"
#include "third_party/blink/renderer/core/paint/compositing/paint_layer_compositor.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/core/paint/paint_layer_scrollable_area.h"
#include "third_party/blink/renderer/platform/graphics/graphics_layer.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/blink/renderer/platform/scroll/scrollable_area.h"

namespace blink {

class RootFrameViewport;

namespace {

bool FillsViewport(const Element& element, bool check_location) {
  if (!element.GetLayoutObject())
    return false;

  LayoutObject* layout_object = element.GetLayoutObject();

  // TODO(bokan): Broken for OOPIF. crbug.com/642378.
  Document& top_document = element.GetDocument().TopDocument();
  if (!top_document.GetLayoutView())
    return false;

  FloatQuad quad = layout_object->LocalToAbsoluteQuad(
      FloatRect(ToLayoutBox(layout_object)->PaddingBoxRect()));

  if (!quad.IsRectilinear())
    return false;

  LayoutRect bounding_box(quad.BoundingBox());

  LayoutSize icb_size =
      LayoutSize(top_document.GetLayoutView()->GetLayoutSize());

  float zoom = top_document.GetFrame()->PageZoomFactor();
  LayoutSize controls_hidden_size = LayoutSize(
      top_document.View()->ViewportSizeForViewportUnits().ScaledBy(zoom));

  if (bounding_box.Size() != icb_size &&
      bounding_box.Size() != controls_hidden_size)
    return false;

  if (!check_location)
    return true;

  return bounding_box.Location() == LayoutPoint::Zero();
}

// If the element is an iframe this grabs the ScrollableArea for the owned
// LayoutView.
PaintLayerScrollableArea* GetScrollableArea(const Element& element) {
  if (element.IsFrameOwnerElement()) {
    const HTMLFrameOwnerElement* frame_owner =
        ToHTMLFrameOwnerElement(&element);
    if (!frame_owner->ContentFrame())
      return nullptr;

    if (!frame_owner->ContentFrame()->IsLocalFrame())
      return nullptr;

    LocalFrameView* frame_view =
        ToLocalFrameView(frame_owner->OwnedEmbeddedContentView());

    if (!frame_view)
      return nullptr;

    return ToPaintLayerScrollableArea(
        frame_view->LayoutViewportScrollableArea());
  }

  DCHECK(element.GetLayoutObject()->IsBox());
  return ToLayoutBox(element.GetLayoutObject())->GetScrollableArea();
}

}  // namespace

// static
RootScrollerController* RootScrollerController::Create(Document& document) {
  return new RootScrollerController(document);
}

RootScrollerController::RootScrollerController(Document& document)
    : document_(&document),
      effective_root_scroller_(&document),
      document_has_document_element_(false) {}

void RootScrollerController::Trace(blink::Visitor* visitor) {
  visitor->Trace(document_);
  visitor->Trace(root_scroller_);
  visitor->Trace(effective_root_scroller_);
  visitor->Trace(implicit_candidates_);
  visitor->Trace(implicit_root_scroller_);
}

void RootScrollerController::Set(Element* new_root_scroller) {
  if (root_scroller_ == new_root_scroller)
    return;

  root_scroller_ = new_root_scroller;

  if (LocalFrame* frame = document_->GetFrame())
    frame->ScheduleVisualUpdateUnlessThrottled();
}

Element* RootScrollerController::Get() const {
  return root_scroller_;
}

Node& RootScrollerController::EffectiveRootScroller() const {
  DCHECK(effective_root_scroller_);
  return *effective_root_scroller_;
}

void RootScrollerController::DidResizeFrameView() {
  DCHECK(document_);

  Page* page = document_->GetPage();
  if (document_->GetFrame() && document_->GetFrame()->IsMainFrame() && page)
    page->GlobalRootScrollerController().DidResizeViewport();

  // If the effective root scroller in this Document is a Frame, it'll match
  // its parent's frame rect. We can't rely on layout to kick it to update its
  // geometry so we do so explicitly here.
  if (EffectiveRootScroller().IsFrameOwnerElement()) {
    UpdateIFrameGeometryAndLayoutSize(
        *ToHTMLFrameOwnerElement(&EffectiveRootScroller()));
  }
}

void RootScrollerController::DidUpdateIFrameFrameView(
    HTMLFrameOwnerElement& element) {
  if (&element != root_scroller_.Get() && &element != implicit_root_scroller_)
    return;

  // Ensure properties are re-applied even if the effective root scroller
  // doesn't change since the FrameView might have been swapped out and the new
  // one should have the properties reapplied.
  if (element.OwnedEmbeddedContentView())
    ApplyRootScrollerProperties(element);

  // Schedule a frame so we can reevaluate whether the iframe should be the
  // effective root scroller (e.g.  demote it if it became remote).
  if (LocalFrame* frame = document_->GetFrame())
    frame->ScheduleVisualUpdateUnlessThrottled();
}

void RootScrollerController::RecomputeEffectiveRootScroller() {
  ProcessImplicitCandidates();

  Node* new_effective_root_scroller = document_;

  if (!DocumentFullscreen::fullscreenElement(*document_)) {
    bool root_scroller_valid =
        root_scroller_ && IsValidRootScroller(*root_scroller_);
    if (root_scroller_valid)
      new_effective_root_scroller = root_scroller_;
    else if (implicit_root_scroller_)
      new_effective_root_scroller = implicit_root_scroller_;
  }

  // TODO(bokan): This is a terrible hack but required because the viewport
  // apply scroll works on Elements rather than Nodes. If we're going from
  // !documentElement to documentElement, we can't early out even if the root
  // scroller didn't change since the global root scroller didn't have an
  // Element previously to put it's ViewportScrollCallback onto. We need this
  // to kick the global root scroller to recompute itself. We can remove this
  // if ScrollCustomization is moved to the Node rather than Element.
  bool old_has_document_element = document_has_document_element_;
  document_has_document_element_ = document_->documentElement();

  if (old_has_document_element || !document_has_document_element_) {
    if (effective_root_scroller_ == new_effective_root_scroller)
      return;
  }

  Node* old_effective_root_scroller = effective_root_scroller_;
  effective_root_scroller_ = new_effective_root_scroller;

  ApplyRootScrollerProperties(*old_effective_root_scroller);
  ApplyRootScrollerProperties(*effective_root_scroller_);

  if (Page* page = document_->GetPage())
    page->GlobalRootScrollerController().DidChangeRootScroller();
}

bool RootScrollerController::IsValidRootScroller(const Element& element) const {
  if (!element.IsInTreeScope())
    return false;

  if (!element.GetLayoutObject())
    return false;

  if (!element.GetLayoutObject()->IsBox())
    return false;

  // Ignore anything inside a FlowThread (multi-col, paginated, etc.).
  if (element.GetLayoutObject()->IsInsideFlowThread())
    return false;

  if (!element.GetLayoutObject()->HasOverflowClip() &&
      !element.IsFrameOwnerElement())
    return false;

  if (element.IsFrameOwnerElement()) {
    const HTMLFrameOwnerElement* frame_owner =
        ToHTMLFrameOwnerElement(&element);

    if (!frame_owner)
      return false;

    if (!frame_owner->OwnedEmbeddedContentView())
      return false;

    // TODO(bokan): Make work with OOPIF. crbug.com/642378.
    if (!frame_owner->OwnedEmbeddedContentView()->IsLocalFrameView())
      return false;
  }

  if (!FillsViewport(element, true))
    return false;

  return true;
}

bool RootScrollerController::IsValidImplicitCandidate(
    const Element& element) const {
  if (!element.IsInTreeScope())
    return false;

  if (!element.GetLayoutObject())
    return false;

  if (!element.GetLayoutObject()->IsBox())
    return false;

  // Ignore anything inside a FlowThread (multi-col, paginated, etc.).
  if (element.GetLayoutObject()->IsInsideFlowThread())
    return false;

  PaintLayerScrollableArea* scrollable_area = GetScrollableArea(element);
  if (!scrollable_area || !scrollable_area->ScrollsOverflow())
    return false;

  return true;
}

bool RootScrollerController::IsValidImplicit(const Element& element) const {
  // Valid implicit root scroller are a subset of valid root scrollers.
  if (!IsValidRootScroller(element))
    return false;

  const ComputedStyle* style = element.GetLayoutObject()->Style();
  if (!style)
    return false;

  // Do not implicitly promote things that are partially or fully invisible.
  if (style->HasOpacity() || style->Visibility() != EVisibility::kVisible)
    return false;

  PaintLayerScrollableArea* scrollable_area = GetScrollableArea(element);
  if (!scrollable_area)
    return false;

  return scrollable_area->ScrollsOverflow();
}

void RootScrollerController::ApplyRootScrollerProperties(Node& node) {
  DCHECK(document_->GetFrame());
  DCHECK(document_->GetFrame()->View());

  // If the node has been removed from the Document, we shouldn't be touching
  // anything related to the Frame- or Layout- hierarchies.
  if (!node.IsInTreeScope())
    return;

  if (!node.IsFrameOwnerElement())
    return;

  HTMLFrameOwnerElement* frame_owner = ToHTMLFrameOwnerElement(&node);

  // The current effective root scroller may have lost its ContentFrame. If
  // that's the case, there's nothing to be done. https://crbug.com/805317 for
  // an example of how we get here.
  if (!frame_owner->ContentFrame())
    return;

  if (frame_owner->ContentFrame()->IsLocalFrame()) {
    LocalFrameView* frame_view =
        ToLocalFrameView(frame_owner->OwnedEmbeddedContentView());

    bool is_root_scroller = &EffectiveRootScroller() == &node;

    // If we're making the Frame the root scroller, it must have a FrameView
    // by now.
    DCHECK(frame_view || !is_root_scroller);
    if (frame_view) {
      frame_view->SetLayoutSizeFixedToFrameSize(!is_root_scroller);
      UpdateIFrameGeometryAndLayoutSize(*frame_owner);
    }
  } else {
    // TODO(bokan): Make work with OOPIF. crbug.com/642378.
  }
}

void RootScrollerController::UpdateIFrameGeometryAndLayoutSize(
    HTMLFrameOwnerElement& frame_owner) const {
  DCHECK(document_->GetFrame());
  DCHECK(document_->GetFrame()->View());

  LocalFrameView* child_view =
      ToLocalFrameView(frame_owner.OwnedEmbeddedContentView());

  if (!child_view)
    return;

  child_view->UpdateGeometry();

  if (&EffectiveRootScroller() == frame_owner)
    child_view->SetLayoutSize(document_->GetFrame()->View()->GetLayoutSize());
}

void RootScrollerController::ProcessImplicitCandidates() {
  implicit_root_scroller_ = nullptr;

  if (!RuntimeEnabledFeatures::ImplicitRootScrollerEnabled())
    return;

  if (!document_->GetLayoutView())
    return;

  // If the document has scrollable content, that's a good sign we shouldn't
  // implicitly promote anything.
  if (document_->GetLayoutView()->GetScrollableArea()->ScrollsOverflow())
    return;

  Element* highest_z_element = nullptr;
  bool highest_is_ambiguous = false;

  HeapHashSet<WeakMember<Element>> copy(implicit_candidates_);
  for (auto& element : copy) {
    if (!IsValidImplicit(*element)) {
      if (!IsValidImplicitCandidate(*element))
        implicit_candidates_.erase(element);
      continue;
    }

    if (!highest_z_element) {
      highest_z_element = element;
    } else {
      int element_z = element->GetLayoutObject()->Style()->ZIndex();
      int highest_z = highest_z_element->GetLayoutObject()->Style()->ZIndex();

      if (element_z > highest_z) {
        highest_z_element = element;
        highest_is_ambiguous = false;
      } else if (element_z == highest_z) {
        highest_is_ambiguous = true;
      }
    }
  }

  if (highest_is_ambiguous)
    implicit_root_scroller_ = nullptr;
  else
    implicit_root_scroller_ = highest_z_element;
}

PaintLayer* RootScrollerController::RootScrollerPaintLayer() const {
  return RootScrollerUtil::PaintLayerForRootScroller(effective_root_scroller_);
}

bool RootScrollerController::ScrollsViewport(const Element& element) const {
  if (effective_root_scroller_->IsDocumentNode())
    return element == document_->documentElement();

  return element == effective_root_scroller_.Get();
}

void RootScrollerController::ElementRemoved(const Element& element) {
  if (element != effective_root_scroller_.Get())
    return;

  effective_root_scroller_ = document_;
  if (Page* page = document_->GetPage())
    page->GlobalRootScrollerController().DidChangeRootScroller();
}

void RootScrollerController::ConsiderForImplicit(Node& node) {
  DCHECK(RuntimeEnabledFeatures::ImplicitRootScrollerEnabled());
  if (!document_->GetFrame()->IsMainFrame())
    return;

  if (document_->GetPage()->GetChromeClient().IsPopup())
    return;

  if (!node.IsElementNode())
    return;

  if (!IsValidImplicitCandidate(ToElement(node)))
    return;

  implicit_candidates_.insert(&ToElement(node));
}

template <typename Function>
void RootScrollerController::ForAllNonThrottledLocalControllers(
    const Function& function) {
  if (!document_->View() || !document_->GetFrame())
    return;

  LocalFrameView* frame_view = document_->View();
  if (frame_view->ShouldThrottleRendering())
    return;

  LocalFrame* frame = document_->GetFrame();
  for (Frame* child = frame->Tree().FirstChild(); child;
       child = child->Tree().NextSibling()) {
    if (!child->IsLocalFrame())
      continue;
    if (Document* child_document = ToLocalFrame(child)->GetDocument()) {
      child_document->GetRootScrollerController()
          .ForAllNonThrottledLocalControllers(function);
    }
  }

  function(*this);
}

void RootScrollerController::PerformRootScrollerSelection() {
  TRACE_EVENT0("blink", "RootScrollerController::PerformRootScrollerSelection");

  // Printing can cause a lifecycle update on a detached frame. In that case,
  // don't make any changes.
  if (!document_->GetFrame() || !document_->GetFrame()->IsLocalRoot())
    return;

  DCHECK(document_->Lifecycle().GetState() >= DocumentLifecycle::kLayoutClean);

  ForAllNonThrottledLocalControllers([](RootScrollerController& controller) {
    controller.RecomputeEffectiveRootScroller();
  });
}

}  // namespace blink
