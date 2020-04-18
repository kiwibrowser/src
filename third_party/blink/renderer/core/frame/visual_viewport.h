/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_VISUAL_VIEWPORT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_VISUAL_VIEWPORT_H_

#include <memory>

#include "base/single_thread_task_runner.h"
#include "third_party/blink/public/platform/web_size.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/page/scrolling/scrolling_coordinator.h"
#include "third_party/blink/renderer/platform/geometry/float_rect.h"
#include "third_party/blink/renderer/platform/geometry/float_size.h"
#include "third_party/blink/renderer/platform/geometry/int_size.h"
#include "third_party/blink/renderer/platform/graphics/compositor_element_id.h"
#include "third_party/blink/renderer/platform/graphics/graphics_layer_client.h"
#include "third_party/blink/renderer/platform/scroll/scroll_types.h"
#include "third_party/blink/renderer/platform/scroll/scrollable_area.h"

namespace blink {

class GraphicsContext;
class GraphicsLayer;
class IntRect;
class IntSize;
class LocalFrame;
class Page;

// Represents the visual viewport the user is currently seeing the page through.
// This class corresponds to the InnerViewport on the compositor. It is a
// ScrollableArea; it's offset is set through the GraphicsLayer <-> CC sync
// mechanisms. Its contents is the page's main LocalFrameView, which corresponds
// to the outer viewport. The inner viewport is always contained in the outer
// viewport and can pan within it.
//
// When attached, the tree will look like this:
//
// VV::m_rootTransformLayer
//  +- VV::m_innerViewportContainerLayer
//     +- VV::m_overscrollElasticityLayer
//     |   +- VV::m_pageScaleLayer
//     |       +- VV::m_innerViewportScrollLayer
//     |           +-- PLC::m_overflowControlsHostLayer
//     |               +-- PLC::m_containerLayer (fixed pos container)
//     |                   +-- PLC::m_scrollLayer
//     |                       +-- PLC::m_rootContentLayer
//     |                           +-- LayoutView CompositedLayerMapping layers
//     +- PageOverlay for InspectorOverlay
//     +- PageOverlay for ColorOverlay
//     +- PLC::m_layerForHorizontalScrollbar
//     +- PLC::m_layerForVerticalScrollbar
//     +- PLC::m_layerForScrollCorner (non-overlay only)
//
class CORE_EXPORT VisualViewport final
    : public GarbageCollectedFinalized<VisualViewport>,
      public GraphicsLayerClient,
      public ScrollableArea {
  USING_GARBAGE_COLLECTED_MIXIN(VisualViewport);

 public:
  static VisualViewport* Create(Page& host) { return new VisualViewport(host); }
  ~VisualViewport() override;

  void Trace(blink::Visitor*) override;

  void CreateLayerTree();
  void AttachLayerTree(GraphicsLayer*);

  GraphicsLayer* RootGraphicsLayer() { return root_transform_layer_.get(); }
  GraphicsLayer* ContainerLayer() {
    return inner_viewport_container_layer_.get();
  }
  GraphicsLayer* ScrollLayer() { return inner_viewport_scroll_layer_.get(); }
  GraphicsLayer* PageScaleLayer() { return page_scale_layer_.get(); }
  GraphicsLayer* OverscrollElasticityLayer() {
    return overscroll_elasticity_layer_.get();
  }

  void InitializeScrollbars();

  // Sets the location of the visual viewport relative to the outer viewport.
  // The coordinates are in partial CSS pixels.
  void SetLocation(const FloatPoint&);
  // FIXME: This should be called moveBy
  void Move(const ScrollOffset&);

  // Sets the size of the inner viewport when unscaled in CSS pixels.
  void SetSize(const IntSize&);
  IntSize Size() const { return size_; }

  // Gets the scaled size, i.e. the viewport in root view space.
  FloatSize VisibleSize() const;

  // Resets the viewport to initial state.
  void Reset();

  // Let the viewport know that the main frame changed size (either through
  // screen rotation on Android or window resize elsewhere).
  void MainFrameDidChangeSize();

  // Sets scale and location in one operation, preventing intermediate clamping.
  void SetScaleAndLocation(float scale, const FloatPoint& location);
  void SetScale(float);
  float Scale() const { return scale_; }

  // Update scale factor, magnifying or minifying by magnifyDelta, centered
  // around the point specified by anchor in window coordinates. Returns false
  // if page scale factor is left unchanged.
  bool MagnifyScaleAroundAnchor(float magnify_delta, const FloatPoint& anchor);

  // The portion of the unzoomed frame visible in the visual viewport,
  // in partial CSS pixels. Relative to the main frame.
  FloatRect VisibleRect() const;

  // The viewport rect relative to the document origin, in partial CSS pixels.
  FloatRect VisibleRectInDocument() const;

  // Convert the given rect in the main LocalFrameView's coordinates into a rect
  // in the viewport. The given and returned rects are in CSS pixels, meaning
  // scale isn't applied.
  FloatPoint ViewportCSSPixelsToRootFrame(const FloatPoint&) const;

  // Clamp the given point, in document coordinates, to the maximum/minimum
  // scroll extents of the viewport within the document.
  IntPoint ClampDocumentOffsetAtScale(const IntPoint& offset, float scale);

  // FIXME: This is kind of a hack. Ideally, we would just resize the
  // viewports to account for browser controls. However, LocalFrameView includes
  // much more than just scrolling so we can't simply resize it without
  // incurring all sorts of side-effects. Until we can seperate out the
  // scrollability aspect from LocalFrameView, we use this method to let
  // VisualViewport make the necessary adjustments so that we don't incorrectly
  // clamp scroll offsets coming from the compositor. crbug.com/422328
  void SetBrowserControlsAdjustment(float);
  float BrowserControlsAdjustment() const;

  // Adjust the viewport's offset so that it remains bounded by the outer
  // viepwort.
  void ClampToBoundaries();

  FloatRect ViewportToRootFrame(const FloatRect&) const;
  IntRect ViewportToRootFrame(const IntRect&) const;
  FloatRect RootFrameToViewport(const FloatRect&) const;
  IntRect RootFrameToViewport(const IntRect&) const;

  FloatPoint ViewportToRootFrame(const FloatPoint&) const;
  FloatPoint RootFrameToViewport(const FloatPoint&) const;
  IntPoint ViewportToRootFrame(const IntPoint&) const;
  IntPoint RootFrameToViewport(const IntPoint&) const;

  // ScrollableArea implementation
  PlatformChromeClient* GetChromeClient() const override;
  bool ShouldUseIntegerScrollOffset() const override;
  void SetScrollOffset(const ScrollOffset&,
                       ScrollType,
                       ScrollBehavior = kScrollBehaviorInstant) override;
  bool IsActive() const override { return false; }
  int ScrollSize(ScrollbarOrientation) const override;
  bool IsScrollCornerVisible() const override { return false; }
  IntRect ScrollCornerRect() const override { return IntRect(); }
  IntSize ScrollOffsetInt() const override { return FlooredIntSize(offset_); }
  ScrollOffset GetScrollOffset() const override { return offset_; }
  IntSize MinimumScrollOffsetInt() const override;
  IntSize MaximumScrollOffsetInt() const override;
  ScrollOffset MaximumScrollOffset() const override;
  int VisibleHeight() const override { return VisibleRect().Height(); }
  int VisibleWidth() const override { return VisibleRect().Width(); }
  IntSize ContentsSize() const override;
  bool ScrollbarsCanBeActive() const override { return false; }
  IntRect ScrollableAreaBoundingBox() const override;
  bool UserInputScrollable(ScrollbarOrientation) const override;
  bool ShouldPlaceVerticalScrollbarOnLeft() const override { return false; }
  CompositorElementId GetCompositorElementId() const override;
  bool ScrollAnimatorEnabled() const override;
  void ScrollControlWasSetNeedsPaintInvalidation() override {}
  void UpdateScrollOffset(const ScrollOffset&, ScrollType) override;
  GraphicsLayer* LayerForContainer() const override;
  GraphicsLayer* LayerForScrolling() const override;
  GraphicsLayer* LayerForHorizontalScrollbar() const override;
  GraphicsLayer* LayerForVerticalScrollbar() const override;
  bool ScheduleAnimation() override;
  CompositorAnimationHost* GetCompositorAnimationHost() const override;
  CompositorAnimationTimeline* GetCompositorAnimationTimeline() const override;
  IntRect VisibleContentRect(
      IncludeScrollbarsInRect = kExcludeScrollbars) const override;
  scoped_refptr<base::SingleThreadTaskRunner> GetTimerTaskRunner() const final;

  // VisualViewport scrolling may involve pinch zoom and gets routed through
  // WebViewImpl explicitly rather than via ScrollingCoordinator::DidScroll
  // since it needs to be set in tandem with the page scale delta.
  void DidScroll(const gfx::ScrollOffset&) final { NOTREACHED(); }

  // Visual Viewport API implementation.
  double OffsetLeft() const;
  double OffsetTop() const;
  double Width() const;
  double Height() const;
  double ScaleForVisualViewport() const;

  // Used to calculate Width and Height above but do not update layout.
  double VisibleWidthCSSPx() const;
  double VisibleHeightCSSPx() const;

  // Used for gathering data on user pinch-zoom statistics.
  void UserDidChangeScale();
  void SendUMAMetrics();
  void StartTrackingPinchStats();

  // Heuristic-based function for determining if we should disable workarounds
  // for viewing websites that are not optimized for mobile devices.
  bool ShouldDisableDesktopWorkarounds() const;

  ScrollbarTheme& GetPageScrollbarTheme() const override;

 private:
  explicit VisualViewport(Page&);

  bool DidSetScaleOrLocation(float scale, const FloatPoint& location);

  bool VisualViewportSuppliesScrollbars() const;

  void UpdateStyleAndLayoutIgnorePendingStylesheets() const;

  void EnqueueScrollEvent();
  void EnqueueResizeEvent();

  // GraphicsLayerClient implementation.
  bool NeedsRepaint(const GraphicsLayer&) const override {
    NOTREACHED();
    return true;
  }
  IntRect ComputeInterestRect(const GraphicsLayer*,
                              const IntRect&) const override;
  void PaintContents(const GraphicsLayer*,
                     GraphicsContext&,
                     GraphicsLayerPaintingPhase,
                     const IntRect&) const override;
  String DebugName(const GraphicsLayer*) const override;

  void SetupScrollbar(ScrollbarOrientation);

  void NotifyRootFrameViewport() const;

  LocalFrame* MainFrame() const;

  Page& GetPage() const {
    DCHECK(page_);
    return *page_;
  }

  Member<Page> page_;
  std::unique_ptr<GraphicsLayer> root_transform_layer_;
  std::unique_ptr<GraphicsLayer> inner_viewport_container_layer_;
  std::unique_ptr<GraphicsLayer> overscroll_elasticity_layer_;
  std::unique_ptr<GraphicsLayer> page_scale_layer_;
  std::unique_ptr<GraphicsLayer> inner_viewport_scroll_layer_;

  // The layers of the ScrollbarLayerGroups are referenced from the
  // GraphicsLayers, so the GraphicsLayers must be destructed first (declared
  // after).
  std::unique_ptr<ScrollingCoordinator::ScrollbarLayerGroup>
      scrollbar_layer_group_horizontal_;
  std::unique_ptr<ScrollingCoordinator::ScrollbarLayerGroup>
      scrollbar_layer_group_vertical_;
  std::unique_ptr<GraphicsLayer> overlay_scrollbar_horizontal_;
  std::unique_ptr<GraphicsLayer> overlay_scrollbar_vertical_;

  // Offset of the visual viewport from the main frame's origin, in CSS pixels.
  ScrollOffset offset_;
  float scale_;
  IntSize size_;
  float browser_controls_adjustment_;
  float max_page_scale_;
  bool track_pinch_zoom_stats_for_page_;
  UniqueObjectId unique_id_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_VISUAL_VIEWPORT_H_
