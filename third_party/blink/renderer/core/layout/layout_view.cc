/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc.
 *               All rights reserved.
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
 */

#include "third_party/blink/renderer/core/layout/layout_view.h"

#include <inttypes.h>

#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/editing/frame_selection.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/html/html_iframe_element.h"
#include "third_party/blink/renderer/core/input/event_handler.h"
#include "third_party/blink/renderer/core/layout/hit_test_result.h"
#include "third_party/blink/renderer/core/layout/layout_counter.h"
#include "third_party/blink/renderer/core/layout/layout_embedded_content.h"
#include "third_party/blink/renderer/core/layout/layout_geometry_map.h"
#include "third_party/blink/renderer/core/layout/svg/layout_svg_root.h"
#include "third_party/blink/renderer/core/layout/view_fragmentation_context.h"
#include "third_party/blink/renderer/core/page/chrome_client.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/paint/compositing/paint_layer_compositor.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/core/paint/view_painter.h"
#include "third_party/blink/renderer/core/svg/svg_document_extensions.h"
#include "third_party/blink/renderer/platform/geometry/float_quad.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_controller.h"
#include "third_party/blink/renderer/platform/histogram.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/traced_value.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/transforms/transform_state.h"

namespace blink {

namespace {

class HitTestLatencyRecorder {
 public:
  HitTestLatencyRecorder(bool allows_child_frame_content)
      : start_(WTF::CurrentTimeTicksInSeconds()),
        allows_child_frame_content_(allows_child_frame_content) {}

  ~HitTestLatencyRecorder() {
    int duration =
        static_cast<int>((WTF::CurrentTimeTicksInSeconds() - start_) * 1000000);

    if (allows_child_frame_content_) {
      DEFINE_STATIC_LOCAL(CustomCountHistogram, recursive_latency_histogram,
                          ("Event.Latency.HitTestRecursive", 0, 10000000, 100));
      recursive_latency_histogram.Count(duration);
    } else {
      DEFINE_STATIC_LOCAL(CustomCountHistogram, latency_histogram,
                          ("Event.Latency.HitTest", 0, 10000000, 100));
      latency_histogram.Count(duration);
    }
  }

 private:
  double start_;
  bool allows_child_frame_content_;
};

}  // namespace

LayoutView::LayoutView(Document* document)
    : LayoutBlockFlow(document),
      frame_view_(document->View()),
      layout_state_(nullptr),
      layout_quote_head_(nullptr),
      layout_counter_count_(0),
      hit_test_count_(0),
      hit_test_cache_hits_(0),
      hit_test_cache_(HitTestCache::Create()) {
  // init LayoutObject attributes
  SetInline(false);

  min_preferred_logical_width_ = LayoutUnit();
  max_preferred_logical_width_ = LayoutUnit();

  SetPreferredLogicalWidthsDirty(kMarkOnlyThis);

  SetPositionState(EPosition::kAbsolute);  // to 0,0 :)
}

LayoutView::~LayoutView() = default;

bool LayoutView::HitTest(HitTestResult& result) {
  // We have to recursively update layout/style here because otherwise, when the
  // hit test recurses into a child document, it could trigger a layout on the
  // parent document, which can destroy PaintLayer that are higher up in the
  // call stack, leading to crashes.
  // Note that Document::updateLayout calls its parent's updateLayout.
  // Note that if an iframe has its render pipeline throttled, it will not
  // update layout here, and it will also not propagate the hit test into the
  // iframe's inner document.
  if (!GetFrameView()->UpdateLifecycleToPrePaintClean())
    return false;
  HitTestLatencyRecorder hit_test_latency_recorder(
      result.GetHitTestRequest().AllowsChildFrameContent());
  return HitTestNoLifecycleUpdate(result);
}

bool LayoutView::HitTestNoLifecycleUpdate(HitTestResult& result) {
  TRACE_EVENT_BEGIN0("blink,devtools.timeline", "HitTest");
  hit_test_count_++;

  DCHECK(!result.GetHitTestLocation().IsRectBasedTest() ||
         result.GetHitTestRequest().ListBased());

  uint64_t dom_tree_version = GetDocument().DomTreeVersion();
  HitTestResult cache_result = result;
  bool hit_layer = false;
  if (hit_test_cache_->LookupCachedResult(cache_result, dom_tree_version)) {
    hit_test_cache_hits_++;
    hit_layer = true;
    result = cache_result;
  } else {
    hit_layer = Layer()->HitTest(result);

    // LocalFrameView scrollbars are not the same as Layer scrollbars tested by
    // Layer::hitTestOverflowControls, so we need to test LocalFrameView
    // scrollbars separately here. Note that it's important we do this after the
    // hit test above, because that may overwrite the entire HitTestResult when
    // it finds a hit.
    IntPoint frame_point = GetFrameView()->ContentsToFrame(
        result.GetHitTestLocation().RoundedPoint());
    if (Scrollbar* frame_scrollbar =
            GetFrameView()->ScrollbarAtFramePoint(frame_point)) {
      result.SetScrollbar(frame_scrollbar);
      hit_layer = true;
    }

    // If hitTestResult include scrollbar, innerNode should be the parent of the
    // scrollbar.
    if (result.GetScrollbar()) {
      // Clear innerNode if we hit a scrollbar whose ScrollableArea isn't
      // associated with a LayoutBox so we aren't hitting some random element
      // below too.
      result.SetInnerNode(nullptr);
      result.SetURLElement(nullptr);
      ScrollableArea* scrollable_area =
          result.GetScrollbar()->GetScrollableArea();
      if (scrollable_area && scrollable_area->GetLayoutBox() &&
          scrollable_area->GetLayoutBox()->GetNode()) {
        Node* node = scrollable_area->GetLayoutBox()->GetNode();

        // If scrollbar belongs to Document, we should set innerNode to the
        // <html> element to match other browser.
        if (node->IsDocumentNode())
          node = node->GetDocument().documentElement();

        result.SetInnerNode(node);
        result.SetURLElement(node->EnclosingLinkEventParentOrSelf());
      }
    }

    if (hit_layer)
      hit_test_cache_->AddCachedResult(result, dom_tree_version);
  }

  TRACE_EVENT_END1(
      "blink,devtools.timeline", "HitTest", "endData",
      InspectorHitTestEvent::EndData(result.GetHitTestRequest(),
                                     result.GetHitTestLocation(), result));
  return hit_layer;
}

void LayoutView::ClearHitTestCache() {
  hit_test_cache_->Clear();
  auto* object = GetFrame()->OwnerLayoutObject();
  if (object)
    object->View()->ClearHitTestCache();
}

void LayoutView::ComputeLogicalHeight(
    LayoutUnit logical_height,
    LayoutUnit,
    LogicalExtentComputedValues& computed_values) const {
  computed_values.extent_ = LayoutUnit(ViewLogicalHeightForBoxSizing());
}

void LayoutView::UpdateLogicalWidth() {
  SetLogicalWidth(LayoutUnit(ViewLogicalWidthForBoxSizing()));
}

bool LayoutView::IsChildAllowed(LayoutObject* child,
                                const ComputedStyle&) const {
  return child->IsBox();
}

bool LayoutView::CanHaveChildren() const {
  FrameOwner* owner = GetFrame()->Owner();
  if (!owner)
    return true;
  if (!RuntimeEnabledFeatures::DisplayNoneIFrameCreatesNoLayoutObjectEnabled())
    return true;
  // Although it is not spec compliant, many websites intentionally call
  // Window.print() on display:none iframes. https://crbug.com/819327.
  if (GetDocument().Printing())
    return true;
  // A PluginDocument needs a layout tree during loading, even if it is inside a
  // display: none iframe.  This is because WebLocalFrameImpl::DidFinish expects
  // the PluginDocument's <embed> element to have an EmbeddedContentView, which
  // it acquires during LocalFrameView::UpdatePlugins, which operates on the
  // <embed> element's layout object (LayoutEmbeddedObject).
  if (GetDocument().IsPluginDocument())
    return true;
  return !owner->IsDisplayNone();
}

#if DCHECK_IS_ON()
void LayoutView::CheckLayoutState() {
  DCHECK(!layout_state_->Next());
}
#endif

void LayoutView::SetShouldDoFullPaintInvalidationOnResizeIfNeeded(
    bool width_changed,
    bool height_changed) {
  // When background-attachment is 'fixed', we treat the viewport (instead of
  // the 'root' i.e. html or body) as the background positioning area, and we
  // should fully invalidate on viewport resize if the background image is not
  // composited and needs full paint invalidation on background positioning area
  // resize.
  if (Style()->HasFixedBackgroundImage()) {
    if ((width_changed && MustInvalidateFillLayersPaintOnWidthChange(
                              Style()->BackgroundLayers())) ||
        (height_changed && MustInvalidateFillLayersPaintOnHeightChange(
                               Style()->BackgroundLayers())))
      SetShouldDoFullPaintInvalidation(PaintInvalidationReason::kBackground);
  }
}

void LayoutView::UpdateBlockLayout(bool relayout_children) {
  SubtreeLayoutScope layout_scope(*this);

  // Use calcWidth/Height to get the new width/height, since this will take the
  // full page zoom factor into account.
  relayout_children |=
      !ShouldUsePrintingLayout() &&
      (!frame_view_ || LogicalWidth() != ViewLogicalWidthForBoxSizing() ||
       LogicalHeight() != ViewLogicalHeightForBoxSizing());

  if (relayout_children) {
    layout_scope.SetChildNeedsLayout(this);
    for (LayoutObject* child = FirstChild(); child;
         child = child->NextSibling()) {
      if (child->IsSVGRoot())
        continue;

      if ((child->IsBox() && ToLayoutBox(child)->HasRelativeLogicalHeight()) ||
          child->Style()->LogicalHeight().IsPercentOrCalc() ||
          child->Style()->LogicalMinHeight().IsPercentOrCalc() ||
          child->Style()->LogicalMaxHeight().IsPercentOrCalc())
        layout_scope.SetChildNeedsLayout(child);
    }

    if (GetDocument().SvgExtensions())
      GetDocument()
          .AccessSVGExtensions()
          .InvalidateSVGRootsWithRelativeLengthDescendents(&layout_scope);
  }

  if (!NeedsLayout())
    return;

  LayoutBlockFlow::UpdateBlockLayout(relayout_children);
}

void LayoutView::UpdateLayout() {
  if (!GetDocument().Paginated())
    SetPageLogicalHeight(LayoutUnit());

  // TODO(wangxianzhu): Move this into ViewPaintInvalidator.
  SetShouldDoFullPaintInvalidationOnResizeIfNeeded(
      OffsetWidth() != GetLayoutSize(kIncludeScrollbars).Width(),
      OffsetHeight() != GetLayoutSize(kIncludeScrollbars).Height());

  if (PageLogicalHeight() && ShouldUsePrintingLayout()) {
    min_preferred_logical_width_ = max_preferred_logical_width_ =
        LogicalWidth();
    if (!fragmentation_context_) {
      fragmentation_context_ =
          std::make_unique<ViewFragmentationContext>(*this);
      pagination_state_changed_ = true;
    }
  } else if (fragmentation_context_) {
    fragmentation_context_.reset();
    pagination_state_changed_ = true;
  }

  DCHECK(!layout_state_);
  LayoutState root_layout_state(*this);

  LayoutBlockFlow::UpdateLayout();

#if DCHECK_IS_ON()
  CheckLayoutState();
#endif
  ClearNeedsLayout();
}

LayoutRect LayoutView::LocalVisualRectIgnoringVisibility() const {
  LayoutRect rect = VisualOverflowRect();
  rect.Unite(LayoutRect(rect.Location(), ViewRect().Size()));
  return rect;
}

void LayoutView::MapLocalToAncestor(const LayoutBoxModelObject* ancestor,
                                    TransformState& transform_state,
                                    MapCoordinatesFlags mode) const {
  if (!ancestor && mode & kUseTransforms &&
      ShouldUseTransformFromContainer(nullptr)) {
    TransformationMatrix t;
    GetTransformFromContainer(nullptr, LayoutSize(), t);
    transform_state.ApplyTransform(t);
  }

  if ((mode & kIsFixed) && frame_view_) {
    transform_state.Move(OffsetForFixedPosition());
    // IsFixed flag is only applicable within this LayoutView.
    mode &= ~kIsFixed;
  }

  if (ancestor == this)
    return;

  if (mode & kTraverseDocumentBoundaries) {
    auto* parent_doc_layout_object = GetFrame()->OwnerLayoutObject();
    if (parent_doc_layout_object) {
      if (!(mode & kInputIsInFrameCoordinates)) {
        transform_state.Move(
            LayoutSize(-GetFrame()->View()->GetScrollOffset()));
      } else {
        // The flag applies to immediate LayoutView only.
        mode &= ~kInputIsInFrameCoordinates;
      }

      transform_state.Move(parent_doc_layout_object->ContentBoxOffset());

      parent_doc_layout_object->MapLocalToAncestor(ancestor, transform_state,
                                                   mode);
    } else {
      GetFrameView()->ApplyTransformForTopFrameSpace(transform_state);
    }
  }
}

const LayoutObject* LayoutView::PushMappingToContainer(
    const LayoutBoxModelObject* ancestor_to_stop_at,
    LayoutGeometryMap& geometry_map) const {
  LayoutSize offset;
  LayoutObject* container = nullptr;

  if (geometry_map.GetMapCoordinatesFlags() & kTraverseDocumentBoundaries) {
    if (auto* parent_doc_layout_object = GetFrame()->OwnerLayoutObject()) {
      offset = -LayoutSize(frame_view_->GetScrollOffset());
      offset += parent_doc_layout_object->ContentBoxOffset();
      container = parent_doc_layout_object;
    }
  }

  // If a container was specified, and was not 0 or the LayoutView, then we
  // should have found it by now unless we're traversing to a parent document.
  DCHECK(!ancestor_to_stop_at || ancestor_to_stop_at == this || container);

  if ((!ancestor_to_stop_at || container) &&
      ShouldUseTransformFromContainer(container)) {
    TransformationMatrix t;
    GetTransformFromContainer(container, LayoutSize(), t);
    geometry_map.Push(this, t, kContainsFixedPosition,
                      OffsetForFixedPosition());
  } else {
    geometry_map.Push(this, offset, 0, OffsetForFixedPosition());
  }

  return container;
}

void LayoutView::MapAncestorToLocal(const LayoutBoxModelObject* ancestor,
                                    TransformState& transform_state,
                                    MapCoordinatesFlags mode) const {
  if (this != ancestor && (mode & kTraverseDocumentBoundaries)) {
    if (auto* parent_doc_layout_object = GetFrame()->OwnerLayoutObject()) {
      // A LayoutView is a containing block for fixed-position elements, so
      // don't carry this state across frames.
      parent_doc_layout_object->MapAncestorToLocal(ancestor, transform_state,
                                                   mode & ~kIsFixed);

      transform_state.Move(parent_doc_layout_object->ContentBoxOffset());
      transform_state.Move(LayoutSize(-GetFrame()->View()->GetScrollOffset()));
    }
  } else {
    DCHECK(this == ancestor || !ancestor);
  }

  if (mode & kIsFixed)
    transform_state.Move(OffsetForFixedPosition());
}

void LayoutView::ComputeSelfHitTestRects(Vector<LayoutRect>& rects,
                                         const LayoutPoint&) const {
  // Record the entire size of the contents of the frame. Note that we don't
  // just use the viewport size (containing block) here because we want to
  // ensure this includes all children (so we can avoid walking them
  // explicitly).
  rects.push_back(LayoutRect(LayoutPoint::Zero(),
                             LayoutSize(GetFrameView()->ContentsSize())));
}

void LayoutView::Paint(const PaintInfo& paint_info,
                       const LayoutPoint& paint_offset) const {
  ViewPainter(*this).Paint(paint_info, paint_offset);
}

void LayoutView::PaintBoxDecorationBackground(const PaintInfo& paint_info,
                                              const LayoutPoint&) const {
  ViewPainter(*this).PaintBoxDecorationBackground(paint_info);
}

static void SetShouldDoFullPaintInvalidationForViewAndAllDescendantsInternal(
    LayoutObject* object) {
  object->SetShouldDoFullPaintInvalidation();
  for (LayoutObject* child = object->SlowFirstChild(); child;
       child = child->NextSibling()) {
    SetShouldDoFullPaintInvalidationForViewAndAllDescendantsInternal(child);
  }
}

void LayoutView::SetShouldDoFullPaintInvalidationForViewAndAllDescendants() {
  if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled())
    SetShouldDoFullPaintInvalidationIncludingNonCompositingDescendants();
  else
    SetShouldDoFullPaintInvalidationForViewAndAllDescendantsInternal(this);
}

void LayoutView::InvalidatePaintForViewAndCompositedLayers() {
  SetShouldDoFullPaintInvalidationIncludingNonCompositingDescendants();

  // The only way we know how to hit these ASSERTS below this point is via the
  // Chromium OS login screen.
  DisableCompositingQueryAsserts disabler;

  if (Compositor()->InCompositingMode())
    Compositor()->FullyInvalidatePaint();
}

bool LayoutView::MapToVisualRectInAncestorSpace(
    const LayoutBoxModelObject* ancestor,
    LayoutRect& rect,
    MapCoordinatesFlags mode,
    VisualRectFlags visual_rect_flags) const {
  bool intersects = true;
  if (MapToVisualRectInAncestorSpaceInternalFastPath(
          ancestor, rect, visual_rect_flags, intersects))
    return intersects;

  TransformState transform_state(TransformState::kApplyTransformDirection,
                                 FloatQuad(FloatRect(rect)));
  intersects = MapToVisualRectInAncestorSpaceInternal(ancestor, transform_state,
                                                      mode, visual_rect_flags);
  transform_state.Flatten();
  rect = LayoutRect(transform_state.LastPlanarQuad().BoundingBox());
  return intersects;
}

bool LayoutView::MapToVisualRectInAncestorSpaceInternal(
    const LayoutBoxModelObject* ancestor,
    TransformState& transform_state,
    VisualRectFlags visual_rect_flags) const {
  return MapToVisualRectInAncestorSpaceInternal(ancestor, transform_state, 0,
                                                visual_rect_flags);
}

bool LayoutView::MapToVisualRectInAncestorSpaceInternal(
    const LayoutBoxModelObject* ancestor,
    TransformState& transform_state,
    MapCoordinatesFlags mode,
    VisualRectFlags visual_rect_flags) const {
  if (mode & kIsFixed)
    transform_state.Move(OffsetForFixedPosition(true));

  // Apply our transform if we have one (because of full page zooming).
  if (Layer() && Layer()->Transform()) {
    transform_state.ApplyTransform(Layer()->CurrentTransform(),
                                   TransformState::kFlattenTransform);
  }

  transform_state.Flatten();

  if (ancestor == this)
    return true;

  Element* owner = GetDocument().LocalOwner();
  if (!owner) {
    LayoutRect rect(transform_state.LastPlanarQuad().BoundingBox());
    bool retval = GetFrameView()->MapToVisualRectInTopFrameSpace(rect);
    transform_state.SetQuad(FloatQuad(FloatRect(rect)));
    return retval;
  }

  if (LayoutBox* obj = owner->GetLayoutBox()) {
    LayoutRect rect(transform_state.LastPlanarQuad().BoundingBox());
    if (!(mode & kInputIsInFrameCoordinates)) {
      // Intersect the viewport with the visual rect.
      LayoutRect view_rectangle = ViewRect();
      if (visual_rect_flags & kEdgeInclusive) {
        if (!rect.InclusiveIntersect(view_rectangle)) {
          transform_state.SetQuad(FloatQuad(FloatRect(rect)));
          return false;
        }
      } else {
        rect.Intersect(view_rectangle);
      }

      // Adjust for scroll offset of the view.
      rect.MoveBy(-view_rectangle.Location());
    }
    // Frames are painted at rounded-int position. Since we cannot efficiently
    // compute the subpixel offset of painting at this point in a a bottom-up
    // walk, round to the enclosing int rect, which will enclose the actual
    // visible rect.
    rect = LayoutRect(EnclosingIntRect(rect));

    // Adjust for frame border.
    rect.Move(obj->ContentBoxOffset());
    transform_state.SetQuad(FloatQuad(FloatRect(rect)));

    return obj->MapToVisualRectInAncestorSpaceInternal(
        ancestor, transform_state, visual_rect_flags);
  }

  // This can happen, e.g., if the iframe element has display:none.
  transform_state.SetQuad(FloatQuad(FloatRect()));
  return false;
}

LayoutSize LayoutView::OffsetForFixedPosition(
    bool include_pending_scroll) const {
  FloatSize adjustment;
  if (frame_view_) {
    adjustment += frame_view_->GetScrollOffset();

    // FIXME: Paint invalidation should happen after scroll updates, so there
    // should be no pending scroll delta.
    // However, we still have paint invalidation during layout, so we can't
    // DCHECK for now. crbug.com/434950.
    // DCHECK(m_frameView->pendingScrollDelta().isZero());
    // If we have a pending scroll, invalidate the previous scroll position.
    if (include_pending_scroll && !frame_view_->PendingScrollDelta().IsZero())
      adjustment -= frame_view_->PendingScrollDelta();
  }

  if (HasOverflowClip())
    adjustment += FloatSize(ScrolledContentOffset());

  return RoundedLayoutSize(adjustment);
}

void LayoutView::AbsoluteRects(Vector<IntRect>& rects,
                               const LayoutPoint& accumulated_offset) const {
  rects.push_back(
      PixelSnappedIntRect(accumulated_offset, LayoutSize(Layer()->Size())));
}

void LayoutView::AbsoluteQuads(Vector<FloatQuad>& quads,
                               MapCoordinatesFlags mode) const {
  quads.push_back(LocalToAbsoluteQuad(
      FloatRect(FloatPoint(), FloatSize(Layer()->Size())), mode));
}

void LayoutView::ClearSelection() {
  frame_view_->GetFrame().Selection().ClearLayoutSelection();
}

void LayoutView::CommitPendingSelection() {
  TRACE_EVENT0("blink", "LayoutView::commitPendingSelection");
  DCHECK(!NeedsLayout());
  frame_view_->GetFrame().Selection().CommitAppearanceIfNeeded();
}

bool LayoutView::ShouldUsePrintingLayout() const {
  if (!GetDocument().Printing() || !frame_view_)
    return false;
  return frame_view_->GetFrame().ShouldUsePrintingLayout();
}

LayoutRect LayoutView::ViewRect() const {
  if (ShouldUsePrintingLayout())
    return LayoutRect(LayoutPoint(), Size());
  if (frame_view_)
    return LayoutRect(frame_view_->VisibleContentRect());
  return LayoutRect();
}

LayoutRect LayoutView::OverflowClipRect(
    const LayoutPoint& location,
    OverlayScrollbarClipBehavior overlay_scrollbar_clip_behavior) const {
  LayoutRect rect = ViewRect();
  if (rect.IsEmpty())
    return LayoutBox::OverflowClipRect(location,
                                       overlay_scrollbar_clip_behavior);

  rect.SetLocation(location);
  if (HasOverflowClip())
    ExcludeScrollbars(rect, overlay_scrollbar_clip_behavior);

  return rect;
}

void LayoutView::CalculateScrollbarModes(ScrollbarMode& h_mode,
                                         ScrollbarMode& v_mode) const {
#define RETURN_SCROLLBAR_MODE(mode) \
  {                                 \
    h_mode = v_mode = mode;         \
    return;                         \
  }

  LocalFrame* frame = GetFrame();
  if (!frame)
    RETURN_SCROLLBAR_MODE(kScrollbarAlwaysOff);

  if (FrameOwner* owner = frame->Owner()) {
    // Setting scrolling="no" on an iframe element disables scrolling.
    if (owner->ScrollingMode() == kScrollbarAlwaysOff)
      RETURN_SCROLLBAR_MODE(kScrollbarAlwaysOff);
  }

  Document& document = GetDocument();
  if (Node* body = document.body()) {
    // Framesets can't scroll.
    if (IsHTMLFrameSetElement(body) && body->GetLayoutObject())
      RETURN_SCROLLBAR_MODE(kScrollbarAlwaysOff);
  }

  if (document.Printing()) {
    // When printing, frame-level scrollbars are never displayed.
    // TODO(szager): Figure out the right behavior when printing an overflowing
    // iframe.  https://bugs.chromium.org/p/chromium/issues/detail?id=777528
    RETURN_SCROLLBAR_MODE(kScrollbarAlwaysOff);
  }

  if (LocalFrameView* frameView = GetFrameView()) {
    // Scrollbars can be disabled by LocalFrameView::setCanHaveScrollbars.
    if (!frameView->CanHaveScrollbars())
      RETURN_SCROLLBAR_MODE(kScrollbarAlwaysOff);
  }

  Element* viewportDefiningElement = document.ViewportDefiningElement();
  if (!viewportDefiningElement)
    RETURN_SCROLLBAR_MODE(kScrollbarAuto);

  LayoutObject* viewport = viewportDefiningElement->GetLayoutObject();
  if (!viewport)
    RETURN_SCROLLBAR_MODE(kScrollbarAuto);

  const ComputedStyle* style = viewport->Style();
  if (!style)
    RETURN_SCROLLBAR_MODE(kScrollbarAuto);

  if (viewport->IsSVGRoot()) {
    // Don't allow overflow to affect <img> and css backgrounds
    if (ToLayoutSVGRoot(viewport)->IsEmbeddedThroughSVGImage())
      RETURN_SCROLLBAR_MODE(kScrollbarAuto);

    // FIXME: evaluate if we can allow overflow for these cases too.
    // Overflow is always hidden when stand-alone SVG documents are embedded.
    if (ToLayoutSVGRoot(viewport)
            ->IsEmbeddedThroughFrameContainingSVGDocument())
      RETURN_SCROLLBAR_MODE(kScrollbarAlwaysOff);
  }

  h_mode = v_mode = kScrollbarAuto;

  EOverflow overflow_x = style->OverflowX();
  EOverflow overflow_y = style->OverflowY();

  bool shouldIgnoreOverflowHidden = false;
  if (Settings* settings = document.GetSettings()) {
    if (settings->GetIgnoreMainFrameOverflowHiddenQuirk() &&
        frame->IsMainFrame())
      shouldIgnoreOverflowHidden = true;
  }
  if (!shouldIgnoreOverflowHidden) {
    if (overflow_x == EOverflow::kHidden)
      h_mode = kScrollbarAlwaysOff;
    if (overflow_y == EOverflow::kHidden)
      v_mode = kScrollbarAlwaysOff;
  }

  if (overflow_x == EOverflow::kScroll)
    h_mode = kScrollbarAlwaysOn;
  if (overflow_y == EOverflow::kScroll)
    v_mode = kScrollbarAlwaysOn;

#undef RETURN_SCROLLBAR_MODE
}

void LayoutView::DispatchFakeMouseMoveEventSoon(EventHandler& event_handler) {
  event_handler.DispatchFakeMouseMoveEventSoon(
      MouseEventManager::FakeMouseMoveReason::kDuringScroll);
}

IntRect LayoutView::DocumentRect() const {
  LayoutRect overflow_rect(LayoutOverflowRect());
  FlipForWritingMode(overflow_rect);
  // TODO(crbug.com/650768): The pixel snapping looks incorrect.
  return PixelSnappedIntRect(overflow_rect);
}

IntSize LayoutView::GetLayoutSize(
    IncludeScrollbarsInRect scrollbar_inclusion) const {
  if (ShouldUsePrintingLayout())
    return IntSize(Size().Width().ToInt(), PageLogicalHeight().ToInt());

  if (!frame_view_)
    return IntSize();

  IntSize result = frame_view_->GetLayoutSize(kIncludeScrollbars);
  if (scrollbar_inclusion == kExcludeScrollbars)
    result =
        frame_view_->LayoutViewportScrollableArea()->ExcludeScrollbars(result);
  return result;
}

int LayoutView::ViewLogicalWidth(
    IncludeScrollbarsInRect scrollbar_inclusion) const {
  return Style()->IsHorizontalWritingMode() ? ViewWidth(scrollbar_inclusion)
                                            : ViewHeight(scrollbar_inclusion);
}

int LayoutView::ViewLogicalHeight(
    IncludeScrollbarsInRect scrollbar_inclusion) const {
  return Style()->IsHorizontalWritingMode() ? ViewHeight(scrollbar_inclusion)
                                            : ViewWidth(scrollbar_inclusion);
}

LayoutUnit LayoutView::ViewLogicalHeightForPercentages() const {
  if (ShouldUsePrintingLayout())
    return PageLogicalHeight();
  return LayoutUnit(ViewLogicalHeight());
}

float LayoutView::ZoomFactor() const {
  return frame_view_->GetFrame().PageZoomFactor();
}

const LayoutBox& LayoutView::RootBox() const {
  Element* document_element = GetDocument().documentElement();
  DCHECK(document_element);
  DCHECK(document_element->GetLayoutObject());
  DCHECK(document_element->GetLayoutObject()->IsBox());
  return ToLayoutBox(*document_element->GetLayoutObject());
}

void LayoutView::UpdateAfterLayout() {
  // Unlike every other layer, the root PaintLayer takes its size from the
  // layout viewport size.  The call to AdjustViewSize() will update the
  // frame's contents size, which will also update the page's minimum scale
  // factor.  The call to ResizeAfterLayout() will calculate the layout viewport
  // size based on the page minimum scale factor, and then update the
  // LocalFrameView with the new size.
  LocalFrame& frame = GetFrameView()->GetFrame();
  if (!GetDocument().Printing())
    GetFrameView()->AdjustViewSize();
  if (frame.IsMainFrame())
    frame.GetChromeClient().ResizeAfterLayout();
  if (HasOverflowClip())
    GetScrollableArea()->ClampScrollOffsetAfterOverflowChange();
  LayoutBlockFlow::UpdateAfterLayout();
}

void LayoutView::UpdateHitTestResult(HitTestResult& result,
                                     const LayoutPoint& point) {
  if (result.InnerNode())
    return;

  Node* node = GetDocument().documentElement();
  if (node) {
    LayoutPoint adjusted_point = point;
    OffsetForContents(adjusted_point);
    result.SetNodeAndPosition(node, adjusted_point);
  }
}

bool LayoutView::UsesCompositing() const {
  return compositor_ && compositor_->StaleInCompositingMode();
}

PaintLayerCompositor* LayoutView::Compositor() {
  if (!compositor_)
    compositor_ = std::make_unique<PaintLayerCompositor>(*this);

  return compositor_.get();
}

void LayoutView::SetIsInWindow(bool is_in_window) {
  if (compositor_)
    compositor_->SetIsInWindow(is_in_window);
}

IntervalArena* LayoutView::GetIntervalArena() {
  if (!interval_arena_)
    interval_arena_ = IntervalArena::Create();
  return interval_arena_.get();
}

bool LayoutView::BackgroundIsKnownToBeOpaqueInRect(const LayoutRect&) const {
  // FIXME: Remove this main frame check. Same concept applies to subframes too.
  if (!GetFrame()->IsMainFrame())
    return false;

  return frame_view_->HasOpaqueBackground();
}

FloatSize LayoutView::ViewportSizeForViewportUnits() const {
  return GetFrameView() ? GetFrameView()->ViewportSizeForViewportUnits()
                        : FloatSize();
}

void LayoutView::WillBeDestroyed() {
  // TODO(wangxianzhu): This is a workaround of crbug.com/570706.
  // Should find and fix the root cause.
  if (PaintLayer* layer = Layer())
    layer->SetNeedsRepaint();
  LayoutBlockFlow::WillBeDestroyed();
  compositor_.reset();
}

void LayoutView::UpdateFromStyle() {
  LayoutBlockFlow::UpdateFromStyle();

  // LayoutView of the main frame is responsible for painting base background.
  if (GetDocument().IsInMainFrame())
    SetHasBoxDecorationBackground(true);
}

LayoutRect LayoutView::DebugRect() const {
  LayoutRect rect;
  LayoutBlock* block = ContainingBlock();
  if (block)
    block->AdjustChildDebugRect(rect);

  rect.SetWidth(LayoutUnit(ViewWidth(kIncludeScrollbars)));
  rect.SetHeight(LayoutUnit(ViewHeight(kIncludeScrollbars)));

  return rect;
}

IntSize LayoutView::ScrolledContentOffset() const {
  DCHECK(HasOverflowClip());
  // FIXME: Return DoubleSize here. crbug.com/414283.
  return GetScrollableArea()->ScrollOffsetInt();
}

bool LayoutView::UpdateLogicalWidthAndColumnWidth() {
  bool relayout_children = LayoutBlockFlow::UpdateLogicalWidthAndColumnWidth();
  // When we're printing, the size of LayoutView is changed outside of layout,
  // so we'll fail to detect any changes here. Just return true.
  return relayout_children || ShouldUsePrintingLayout();
}

void LayoutView::UpdateCounters() {
  if (!needs_counter_update_)
    return;

  needs_counter_update_ = false;
  if (!HasLayoutCounters())
    return;

  for (LayoutObject* layout_object = this; layout_object;
       layout_object = layout_object->NextInPreOrder()) {
    if (!layout_object->IsCounter())
      continue;

    ToLayoutCounter(layout_object)->UpdateCounter();
  }
}

}  // namespace blink
