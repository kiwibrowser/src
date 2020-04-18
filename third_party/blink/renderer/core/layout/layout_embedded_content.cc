/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Simon Hausmann <hausmann@kde.org>
 *           (C) 2000 Stefan Schimanski (1Stein@gmx.de)
 * Copyright (C) 2004, 2005, 2006, 2009 Apple Inc. All rights reserved.
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "third_party/blink/renderer/core/layout/layout_embedded_content.h"

#include "third_party/blink/renderer/core/dom/ax_object_cache.h"
#include "third_party/blink/renderer/core/exported/web_plugin_container_impl.h"
#include "third_party/blink/renderer/core/frame/embedded_content_view.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/remote_frame_view.h"
#include "third_party/blink/renderer/core/html/html_frame_element_base.h"
#include "third_party/blink/renderer/core/html/html_plugin_element.h"
#include "third_party/blink/renderer/core/layout/hit_test_result.h"
#include "third_party/blink/renderer/core/layout/layout_analyzer.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/page/scrolling/root_scroller_util.h"
#include "third_party/blink/renderer/core/paint/embedded_content_painter.h"

namespace blink {

LayoutEmbeddedContent::LayoutEmbeddedContent(Element* element)
    : LayoutReplaced(element),
      // Reference counting is used to prevent the part from being destroyed
      // while inside the EmbeddedContentView code, which might not be able to
      // handle that.
      ref_count_(1) {
  DCHECK(element);
  SetInline(false);
}

void LayoutEmbeddedContent::Release() {
  if (--ref_count_ <= 0)
    delete this;
}

void LayoutEmbeddedContent::WillBeDestroyed() {
  if (AXObjectCache* cache = GetDocument().ExistingAXObjectCache()) {
    cache->ChildrenChanged(Parent());
    cache->Remove(this);
  }

  Node* node = GetNode();
  if (node && node->IsFrameOwnerElement())
    ToHTMLFrameOwnerElement(node)->SetEmbeddedContentView(nullptr);

  LayoutReplaced::WillBeDestroyed();
}

void LayoutEmbeddedContent::Destroy() {
  WillBeDestroyed();
  // We call clearNode here because LayoutEmbeddedContent is ref counted. This
  // call to destroy may not actually destroy the layout object. We can keep it
  // around because of references from the LocalFrameView class. (The actual
  // destruction of the class happens in PostDestroy() which is called from
  // Release()).
  //
  // But, we've told the system we've destroyed the layoutObject, which happens
  // when the DOM node is destroyed. So there is a good change the DOM node this
  // object points too is invalid, so we have to clear the node so we make sure
  // we don't access it in the future.
  ClearNode();
  Release();
}

LayoutEmbeddedContent::~LayoutEmbeddedContent() {
  DCHECK_LE(ref_count_, 0);
}

FrameView* LayoutEmbeddedContent::ChildFrameView() const {
  EmbeddedContentView* embedded_content_view = GetEmbeddedContentView();

  if (embedded_content_view && embedded_content_view->IsFrameView())
    return ToFrameView(embedded_content_view);

  return nullptr;
}

WebPluginContainerImpl* LayoutEmbeddedContent::Plugin() const {
  EmbeddedContentView* embedded_content_view = GetEmbeddedContentView();
  if (embedded_content_view && embedded_content_view->IsPluginView())
    return ToWebPluginContainerImpl(embedded_content_view);
  return nullptr;
}

EmbeddedContentView* LayoutEmbeddedContent::GetEmbeddedContentView() const {
  Node* node = GetNode();
  if (node && node->IsFrameOwnerElement())
    return ToHTMLFrameOwnerElement(node)->OwnedEmbeddedContentView();
  return nullptr;
}

PaintLayerType LayoutEmbeddedContent::LayerTypeRequired() const {
  PaintLayerType type = LayoutReplaced::LayerTypeRequired();
  if (type != kNoPaintLayer)
    return type;
  return kForcedPaintLayer;
}

bool LayoutEmbeddedContent::RequiresAcceleratedCompositing() const {
  // There are two general cases in which we can return true. First, if this is
  // a plugin LayoutObject and the plugin has a layer, then we need a layer.
  // Second, if this is a LayoutObject with a contentDocument and that document
  // needs a layer, then we need a layer.
  WebPluginContainerImpl* plugin_view = Plugin();
  if (plugin_view && plugin_view->CcLayer())
    return true;

  if (!GetNode() || !GetNode()->IsFrameOwnerElement())
    return false;

  HTMLFrameOwnerElement* element = ToHTMLFrameOwnerElement(GetNode());
  if (element->ContentFrame() && element->ContentFrame()->IsRemoteFrame())
    return true;

  if (Document* content_document = element->contentDocument()) {
    auto* layout_view = content_document->GetLayoutView();
    if (layout_view)
      return layout_view->UsesCompositing();
  }

  return false;
}

bool LayoutEmbeddedContent::NodeAtPointOverEmbeddedContentView(
    HitTestResult& result,
    const HitTestLocation& location_in_container,
    const LayoutPoint& accumulated_offset,
    HitTestAction action) {
  bool had_result = result.InnerNode();
  bool inside = LayoutReplaced::NodeAtPoint(result, location_in_container,
                                            accumulated_offset, action);

  // Check to see if we are really over the EmbeddedContentView itself (and not
  // just in the border/padding area).
  if ((inside || result.IsRectBasedTest()) && !had_result &&
      result.InnerNode() == GetNode()) {
    result.SetIsOverEmbeddedContentView(
        ContentBoxRect().Contains(result.LocalPoint()));
  }
  return inside;
}

bool LayoutEmbeddedContent::NodeAtPoint(
    HitTestResult& result,
    const HitTestLocation& location_in_container,
    const LayoutPoint& accumulated_offset,
    HitTestAction action) {
  FrameView* frame_view = ChildFrameView();
  if (!frame_view || !frame_view->IsLocalFrameView() ||
      !result.GetHitTestRequest().AllowsChildFrameContent()) {
    return NodeAtPointOverEmbeddedContentView(result, location_in_container,
                                              accumulated_offset, action);
  }

  LocalFrameView* local_frame_view = ToLocalFrameView(frame_view);

  // A hit test can never hit an off-screen element; only off-screen iframes are
  // throttled; therefore, hit tests can skip descending into throttled iframes.
  if (local_frame_view->ShouldThrottleRendering()) {
    return NodeAtPointOverEmbeddedContentView(result, location_in_container,
                                              accumulated_offset, action);
  }

  DCHECK_GE(GetDocument().Lifecycle().GetState(),
            DocumentLifecycle::kCompositingClean);

  if (action == kHitTestForeground) {
    auto* child_layout_view = local_frame_view->GetLayoutView();

    if (VisibleToHitTestRequest(result.GetHitTestRequest()) &&
        child_layout_view) {
      LayoutPoint adjusted_location = accumulated_offset + Location();
      LayoutPoint content_offset =
          LayoutPoint(BorderLeft() + PaddingLeft(),
                      BorderTop() + PaddingTop()) -
          LayoutSize(local_frame_view->ScrollOffsetInt());
      HitTestLocation new_hit_test_location(
          location_in_container, -adjusted_location - content_offset);
      HitTestRequest new_hit_test_request(result.GetHitTestRequest().GetType() |
                                          HitTestRequest::kChildFrameHitTest);
      HitTestResult child_frame_result(new_hit_test_request,
                                       new_hit_test_location);

      // The frame's layout and style must be up to date if we reach here.
      bool is_inside_child_frame =
          child_layout_view->HitTestNoLifecycleUpdate(child_frame_result);

      if (result.GetHitTestRequest().ListBased()) {
        result.Append(child_frame_result);
      } else if (is_inside_child_frame) {
        // Force the result not to be cacheable because the parent frame should
        // not cache this result; as it won't be notified of changes in the
        // child.
        child_frame_result.SetCacheable(false);
        result = child_frame_result;
      }

      // Don't trust |isInsideChildFrame|. For rect-based hit-test, returns
      // true only when the hit test rect is totally within the iframe,
      // i.e. nodeAtPointOverEmbeddedContentView() also returns true.
      // Use a temporary HitTestResult because we don't want to collect the
      // iframe element itself if the hit-test rect is totally within the
      // iframe.
      if (is_inside_child_frame) {
        if (!location_in_container.IsRectBasedTest())
          return true;
        HitTestResult point_over_embedded_content_view_result = result;
        bool point_over_embedded_content_view =
            NodeAtPointOverEmbeddedContentView(
                point_over_embedded_content_view_result, location_in_container,
                accumulated_offset, action);
        if (point_over_embedded_content_view)
          return true;
        result = point_over_embedded_content_view_result;
        return false;
      }
    }
  }

  return NodeAtPointOverEmbeddedContentView(result, location_in_container,
                                            accumulated_offset, action);
}

CompositingReasons LayoutEmbeddedContent::AdditionalCompositingReasons() const {
  if (RequiresAcceleratedCompositing())
    return CompositingReason::kIFrame;
  return CompositingReason::kNone;
}

void LayoutEmbeddedContent::StyleDidChange(StyleDifference diff,
                                           const ComputedStyle* old_style) {
  LayoutReplaced::StyleDidChange(diff, old_style);
  EmbeddedContentView* embedded_content_view = GetEmbeddedContentView();
  if (!embedded_content_view)
    return;

  // If the iframe has custom scrollbars, recalculate their style.
  if (FrameView* frame_view = ChildFrameView()) {
    if (frame_view->IsLocalFrameView())
      ToLocalFrameView(frame_view)->RecalculateCustomScrollbarStyle();
  }

  if (Style()->Visibility() != EVisibility::kVisible) {
    embedded_content_view->Hide();
  } else {
    embedded_content_view->Show();
  }
}

void LayoutEmbeddedContent::UpdateLayout() {
  DCHECK(NeedsLayout());
  LayoutAnalyzer::Scope analyzer(*this);
  UpdateAfterLayout();
  ClearNeedsLayout();
}

void LayoutEmbeddedContent::Paint(const PaintInfo& paint_info,
                                  const LayoutPoint& paint_offset) const {
  EmbeddedContentPainter(*this).Paint(paint_info, paint_offset);
}

void LayoutEmbeddedContent::PaintContents(
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset) const {
  EmbeddedContentPainter(*this).PaintContents(paint_info, paint_offset);
}

CursorDirective LayoutEmbeddedContent::GetCursor(const LayoutPoint& point,
                                                 Cursor& cursor) const {
  if (Plugin()) {
    // A plugin is responsible for setting the cursor when the pointer is over
    // it.
    return kDoNotSetCursor;
  }
  return LayoutReplaced::GetCursor(point, cursor);
}

LayoutRect LayoutEmbeddedContent::ReplacedContentRect() const {
  // We don't propagate sub-pixel into sub-frame layout, in other words, the
  // rect is snapped at the document boundary, and sub-pixel movement could
  // cause the sub-frame to layout due to the 1px snap difference. In order to
  // avoid that, the size of sub-frame is rounded in advance.
  LayoutRect size_rounded_rect = ContentBoxRect();

  // IFrames set as the root scroller should get their size from their parent.
  if (ChildFrameView() && View() && RootScrollerUtil::IsEffective(*this))
    size_rounded_rect = LayoutRect(LayoutPoint(), View()->ViewRect().Size());

  size_rounded_rect.SetSize(
      LayoutSize(RoundedIntSize(size_rounded_rect.Size())));
  return size_rounded_rect;
}

void LayoutEmbeddedContent::UpdateOnEmbeddedContentViewChange() {
  EmbeddedContentView* embedded_content_view = GetEmbeddedContentView();
  if (!embedded_content_view)
    return;

  if (!Style())
    return;

  if (!NeedsLayout())
    UpdateGeometry(*embedded_content_view);

  if (Style()->Visibility() != EVisibility::kVisible) {
    embedded_content_view->Hide();
  } else {
    embedded_content_view->Show();
    // FIXME: Why do we issue a full paint invalidation in this case, but not
    // the other?
    SetShouldDoFullPaintInvalidation();
  }
}

void LayoutEmbeddedContent::UpdateGeometry(
    EmbeddedContentView& embedded_content_view) {
  // Ignore transform here, as we only care about the sub-pixel accumulation.
  // TODO(trchen): What about multicol? Need a LayoutBox function to query
  // sub-pixel accumulation.
  LayoutRect replaced_rect = ReplacedContentRect();
  TransformState transform_state(TransformState::kApplyTransformDirection,
                                 FloatPoint(),
                                 FloatQuad(FloatRect(replaced_rect)));
  MapLocalToAncestor(nullptr, transform_state,
                     kApplyContainerFlip | kUseTransforms);
  transform_state.Flatten();
  LayoutPoint absolute_location(transform_state.LastPlanarPoint());
  LayoutRect absolute_replaced_rect(replaced_rect);
  absolute_replaced_rect.MoveBy(absolute_location);
  FloatRect absolute_bounding_box =
      transform_state.LastPlanarQuad().BoundingBox();
  IntRect frame_rect(IntPoint(),
                     PixelSnappedIntRect(absolute_replaced_rect).Size());
  // Normally the location of the frame rect is ignored by the painter, but
  // currently it is still used by a family of coordinate conversion function in
  // LocalFrameView. This is incorrect because coordinate conversion
  // needs to take transform and into account. A few callers still use the
  // family of conversion function, including but not exhaustive:
  // LocalFrameView::updateViewportIntersectionIfNeeded()
  // RemoteFrameView::frameRectsChanged().
  // WebPluginContainerImpl::reportGeometry()
  // TODO(trchen): Remove this hack once we fixed all callers.
  frame_rect.SetLocation(RoundedIntPoint(absolute_bounding_box.Location()));

  // As an optimization, we don't include the root layer's scroll offset in the
  // frame rect.  As a result, we don't need to recalculate the frame rect every
  // time the root layer scrolls; however, each implementation of
  // EmbeddedContentView::FrameRect() must add the root layer's scroll offset
  // into its position.
  // TODO(szager): Refactor this functionality into EmbeddedContentView, rather
  // than reimplementing in each concrete subclass.
  LayoutView* layout_view = View();
  if (layout_view && layout_view->HasOverflowClip())
    frame_rect.Move(layout_view->ScrolledContentOffset());

  embedded_content_view.SetFrameRect(frame_rect);
}

bool LayoutEmbeddedContent::IsThrottledFrameView() const {
  FrameView* frame_view = ChildFrameView();
  if (frame_view && frame_view->IsLocalFrameView())
    return ToLocalFrameView(frame_view)->ShouldThrottleRendering();
  return false;
}

}  // namespace blink
