// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/page/scrolling/root_scroller_util.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/dom/node.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/layout/layout_box.h"
#include "third_party/blink/renderer/core/layout/layout_box_model_object.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/page/scrolling/root_scroller_controller.h"
#include "third_party/blink/renderer/core/page/scrolling/top_document_root_scroller_controller.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/core/paint/paint_layer_scrollable_area.h"

namespace blink {

namespace RootScrollerUtil {

ScrollableArea* ScrollableAreaForRootScroller(const Node* node) {
  if (!node)
    return nullptr;

  if (node->IsDocumentNode() || node == node->GetDocument().documentElement()) {
    if (!node->GetDocument().View())
      return nullptr;

    // For a FrameView, we use the layoutViewport rather than the
    // getScrollableArea() since that could be the RootFrameViewport. The
    // rootScroller's ScrollableArea will be swapped in as the layout viewport
    // in RootFrameViewport so we need to ensure we get the layout viewport.
    return node->GetDocument().View()->LayoutViewportScrollableArea();
  }

  DCHECK(node->IsElementNode());
  const Element* element = ToElement(node);

  if (!element->GetLayoutObject() || !element->GetLayoutObject()->IsBox())
    return nullptr;

  return static_cast<PaintInvalidationCapableScrollableArea*>(
      ToLayoutBoxModelObject(element->GetLayoutObject())->GetScrollableArea());
}

PaintLayer* PaintLayerForRootScroller(const Node* node) {
  if (!node)
    return nullptr;

  if (node->IsDocumentNode() || node == node->GetDocument().documentElement()) {
    if (!node->GetDocument().GetLayoutView())
      return nullptr;

    return node->GetDocument().GetLayoutView()->Layer();
  }

  DCHECK(node->IsElementNode());
  const Element* element = ToElement(node);
  if (!element->GetLayoutObject() || !element->GetLayoutObject()->IsBox())
    return nullptr;

  LayoutBox* box = ToLayoutBox(element->GetLayoutObject());
  return box->Layer();
}

bool IsEffective(const LayoutBox& box) {
  if (!box.GetNode())
    return false;

  return box.GetNode() ==
         &box.GetDocument().GetRootScrollerController().EffectiveRootScroller();
}

bool IsGlobal(const LayoutBox& box) {
  if (!box.GetNode() || !box.GetNode()->GetDocument().GetPage())
    return false;

  return box.GetNode() == box.GetDocument()
                              .GetPage()
                              ->GlobalRootScrollerController()
                              .GlobalRootScroller();
}

bool IsEffective(const PaintLayer& layer) {
  if (!layer.GetLayoutBox())
    return false;

  return IsEffective(*layer.GetLayoutBox());
}

bool IsGlobal(const PaintLayer& layer) {
  if (!layer.GetLayoutBox())
    return false;

  PaintLayer* root_scroller_layer =
      PaintLayerForRootScroller(layer.GetLayoutBox()
                                    ->GetDocument()
                                    .GetPage()
                                    ->GlobalRootScrollerController()
                                    .GlobalRootScroller());

  return &layer == root_scroller_layer;
}

bool IsGlobal(const Element* element) {
  return element->GetDocument()
             .GetPage()
             ->GlobalRootScrollerController()
             .GlobalRootScroller() == element;
}

}  // namespace RootScrollerUtil

}  // namespace blink
